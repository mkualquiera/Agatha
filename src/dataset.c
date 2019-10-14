#if INTERFACE

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <float.h>
#include "debug.h"

typedef enum DataType {
  DISCRETE,
  CONTINUOUS
} DataType;

typedef union DatasetValue {
  int discrete;
  double continous;
} DatasetValue;

typedef struct DatasetFeature {
  char *name;
  DataType type;
  bool is_label;
  int discrete_possibility_count;
  int *discrete_possibles;
  double continuous_lower_boundary;
  double continuous_upper_boundary;
  int continuous_precision;
} DatasetFeature;

#define FEATURE_COUNT_STEP 16
typedef struct DatasetHeader {
  DatasetFeature *label;
  unsigned int label_index;
  DatasetFeature **features;
  unsigned short max_feature_count;
  unsigned short feature_count;
} DatasetHeader;

typedef struct Dataset Dataset;
typedef struct DatasetEntry DatasetEntry;

typedef struct DatasetEntry {
  DatasetValue *values;
  DatasetEntry *previous;
  DatasetEntry *next;
} DatabaseEntry;

typedef struct Dataset {
  bool has_cached_counts;
  unsigned int *counts;
  DatasetHeader *header;
  DatasetEntry *head;
  DatasetEntry *tail;
  unsigned short entry_count;
} Dataset;

#define DATASETS_FOLDER "datasets"
#define DATA_FILENAME "data"
#define NAMES_FILENAME "names"

#endif

#include "dataset.h"

void dataset_entry_dispose(DatasetEntry *entry) {
  free(entry->values);
  free(entry);
}

DatasetEntry* dataset_entry_create(unsigned int feature_count) {
  if (feature_count <= 1) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "WARNING !! Incorrect number of features for a dataset entry.");
    }
    return NULL;
  }
  DatasetEntry *result = calloc(1, sizeof(*result));
  result->values = calloc(feature_count, sizeof(DatasetValue));
  return result;
}



void dataset_dispose(Dataset *dataset, bool free_header, bool free_data) {
  if (free_header) {
    dataset_header_dispose(dataset->header);
  }
  if (free_data) {
    DatasetEntry *focus = dataset->head;
    while((focus != dataset->tail) & (focus != NULL)) {
      DatasetEntry *next = focus->next;
      dataset_entry_dispose(focus);
      focus = next;
    }
    dataset_entry_dispose(focus);
  }
  free(dataset);
}

Dataset* dataset_create(DatasetHeader* header) {
  if (header == NULL) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "WARNING !! Tried to create a dataset with a null header.");
    }
    return NULL;
  }
  Dataset *result = calloc(1, sizeof(*result));
  result->header = header;
  result->has_cached_counts = false;
  result->head = NULL;
  result->tail = NULL;
  return result;
}

void dataset_feature_dispose(DatasetFeature* target) {
  free(target->name);
  free(target->discrete_possibles);
  free(target);
}

void dataset_header_dispose(DatasetHeader* header) {
  for(unsigned int i = 0; i < header->feature_count; i++) {
    dataset_feature_dispose(header->features[i]);
  }
  free(header->features);
  free(header);
}

DatasetHeader* dataset_header_create() {
  DatasetHeader *result = calloc(1, sizeof(*result));
  result->max_feature_count = FEATURE_COUNT_STEP;
  result->features = calloc(result->max_feature_count, sizeof(DatasetFeature*));
  return result;
}

void dataset_header_add_feature(DatasetHeader* header, DatasetFeature* feature) {
  // Check if the feature is valid.
  if (feature == NULL) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "WARNING !! Tried to add a NULL feature to a header.");
    }
    return;
  }
  // Check if the feature list is full.
  if(header->feature_count == header->max_feature_count) {
    // Increment the maximum feature count.
    header->max_feature_count += FEATURE_COUNT_STEP;
    // Allocate the new feature list and copy the old one into it.
    DatasetFeature** newFeatures = calloc(header->max_feature_count,sizeof(DatasetFeature*));
    for(unsigned int i = 0; i < header->feature_count; i++) {
      newFeatures[i] = header->features[i];
    }
    // Free the old feature list.
    free(header->features);
    header->features = newFeatures;
  }
  // Add the feature to the list.
  header->features[header->feature_count] = feature;
  header->feature_count++;
}

Dataset* dataset_load_from_disk(char* name) {
  // Obtain the length of filepaths so the buffers can be allocated properly.
  unsigned int base_path_length = strlen(DATASETS_FOLDER) + 1 + strlen(name) + 1 + 1;
  unsigned int data_path_length = base_path_length + strlen(DATA_FILENAME);
  unsigned int names_path_length = base_path_length + strlen(NAMES_FILENAME);
  // Allocate the buffers for storing the file paths.
  char *data_path = calloc(data_path_length, sizeof(char*));
  char *names_path = calloc(names_path_length, sizeof(char*));
  // Build the paths into the allocated buffers.
  snprintf(data_path, data_path_length, "%s/%s/%s", DATASETS_FOLDER, name, DATA_FILENAME);
  snprintf(names_path, names_path_length, "%s/%s/%s", DATASETS_FOLDER, name, NAMES_FILENAME);
  // Print the paths for debug purposes.
  if (DEBUG) {
    printf("[%s : %d] %s\n", __FILE__, __LINE__, data_path);
    printf("[%s : %d] %s\n", __FILE__, __LINE__, names_path);
  }
  // Verify access to the files.
  if (access(data_path, R_OK) == -1) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "Unable to read data file.");
    }
    return NULL;
  }
  // Allocate memory for the header.
  DatasetHeader *header = dataset_header_create();
  // Get the handle for the file.
  FILE *fp = fopen(names_path, "r");
  // Verify the file was opened before operating on it.
  if (fp == NULL) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "Unable to read names file.");
    }
    free(names_path);
    free(data_path);
    free(header);
    return NULL;
  }
  // Read the file to load the names into the header.
  bool found_label = false;
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;
  // Parse lines to obtain the features.
  while ((read = getline(&line, &len, fp)) != -1) {
    // Allocate the memory for the feature
    DatasetFeature *feature = calloc(1, sizeof(*feature));
    feature->continuous_lower_boundary = DBL_MAX;
    feature->continuous_upper_boundary = DBL_MIN;
    unsigned char column_id = 0;
    // Tokenize the line for parsing.
    char *token = strtok(line, " ");
    while (token != NULL) {
      if (column_id == 0) {
        // Copy the name into the feature.
        feature->name = calloc(strlen(token) + 1, sizeof(char));
        strcpy(feature->name, token);
      }
      if (column_id == 1) {
        // Set the type of the feature.
        if (token[0] == 'c') {
          feature->type = CONTINUOUS;
        }
        if (token[0] == 'd') {
          feature->type = DISCRETE;
        }
      }
      if (column_id > 1) {
        // Verify that the feature is discrete before adding discrete properties to it.
        if (feature->type == DISCRETE) {
          if (column_id == 2) {
            // Identify if this discrete feature is the label.
            if(token[0] == 'y') {
              // Verify that another feature is not the label already.
              if (!found_label) {
                header->label = feature;
                header->label_index = header->feature_count;
                feature->is_label = true;
                found_label = true;
              } else {
                if (DEBUG) {
                  printf("[%s : %d] %s\n", __FILE__, __LINE__, "WARNING !! Trying to add more than one discrete label.");
                }
              }
            }
          }
          if (column_id == 3) {
            // Obtain the number of discrete possible values for this feature.
            char* end;
            unsigned int pos_count = strtol(token, &end, 10);
            feature->discrete_possibility_count = pos_count;
            feature->discrete_possibles = calloc(pos_count, sizeof(int));
          }
          if (column_id >= 4) {
            // Add the discrete possibilities to the feature.
            unsigned int index = column_id - 4;
            // Make sure the possibility count is not overflowing.
            if (index < feature->discrete_possibility_count) {
              char* end;
              feature->discrete_possibles[index] = strtol(token, &end, 10);
            } else {
              if (DEBUG) {
                printf("[%s : %d] %s\n", __FILE__, __LINE__, "WARNING !! Trying to add more discrete possibilities than the original specified amount.");
              }
            }
          }
        } else {
          if (DEBUG) {
            printf("[%s : %d] %s\n", __FILE__, __LINE__, "WARNING !! Trying to add discrete properties to continuous feature.");
          }
        }
      }
      column_id++;
      // Obtain the next token in the string.
      token = strtok(NULL, " ");
    }
    // Check if the feature even had a name. Can't work otherwise.
    if (feature->name == NULL) {
      if (DEBUG) {
        printf("[%s : %d] %s\n", __FILE__, __LINE__, "Unable to parse names file.");
        return NULL;
      }
      free(feature);
      free(names_path);
      free(data_path);
      dataset_header_dispose(header);
    }
    // Add the feature to the dataset header.
    dataset_header_add_feature(header, feature);
  }
  // Close the file after parsing.
  fclose(fp);
  free(line);
  if (!found_label | (header->feature_count < 2)) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "The dataset must have at least two features and a label.");
    }
    free(names_path);
    free(data_path);
    dataset_header_dispose(header);
    return NULL;
  }
  // Debug the header to make sure it's correct.
  if (DEBUG) {
    printf("[%s : %d] %s %u\n", __FILE__, __LINE__, "Header features:", header->feature_count);
    for (unsigned int i = 0; i < header->feature_count; i++) {
      printf("[%s : %d] -------------\n", __FILE__, __LINE__);
      printf("[%s : %d] Name: %s\n", __FILE__, __LINE__, header->features[i]->name);
      printf("[%s : %d] Type: %u\n", __FILE__, __LINE__, header->features[i]->type);
      printf("[%s : %d] Is label: %u\n", __FILE__, __LINE__, header->features[i]->is_label);
      printf("[%s : %d] Possibility count (discrete): %u\n", __FILE__, __LINE__, header->features[i]->discrete_possibility_count);
    }
  }
  // Allocate memory for the dataset.
  Dataset *dataset = dataset_create(header);
  // Get the handle for the file.
  fp = fopen(data_path, "r");
  // Verify the file was opened before operating on it.
  if (fp == NULL) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "Unable to read data file.");
    }
    free(names_path);
    free(data_path);
    dataset_dispose(dataset, true, true);
    return NULL;
  }
  line = NULL;
  len = 0;
  read = 0;
  unsigned short count = 0;
  DatasetEntry *previous = NULL;
  // Parse lines to fill the dataset.
  while ((read = getline(&line, &len, fp)) != -1) {
    // Ignore empty lines.
    if (read <= 1) {
      continue;
    }
    // Allocate the memory for the dataset.
    DatasetEntry *entry = dataset_entry_create(dataset->header->feature_count);
    unsigned int column_id = 0;
    // Set the entry as the head if it's the first one.
    if (count == 0) {
      dataset->head = entry;
    } else {
      previous->next = entry;
      entry->previous = previous;
    }
    // Tokenize the string using commas (CSV format).
    char *token = strtok(line, ",");
    while (token != NULL) {
      // Determine how to add the value depending on the feature type/
      if(dataset->header->features[column_id]->type == CONTINUOUS) {
        char *end;
        // Parse the token into a double.
        double value = strtod(token, &end);
        entry->values[column_id].continous = value;
        // Check if value goes below bounds.
        if(dataset->header->features[column_id]->continuous_lower_boundary > value) {
          dataset->header->features[column_id]->continuous_lower_boundary = value;
        }
        // Check if value goes above bounds.
        if(dataset->header->features[column_id]->continuous_upper_boundary < value) {
          dataset->header->features[column_id]->continuous_upper_boundary = value;
        }
        bool found_dot = false;
        unsigned short precision = 0;
        // Determine the precision of the value by counting the number of decimals.
        for (unsigned short i = 0; token[i] != '\0'; i++) {
          if (found_dot) {
            precision++;
          }
          if (token[i] == '.') {
            found_dot = true;
          }
        }
        if (dataset->header->features[column_id]->continuous_precision < precision) {
          dataset->header->features[column_id]->continuous_precision = precision;
        }
      } else {
        char *end;
        // Convert token into long, then cast into int.
        unsigned int value = strtol(token, &end, 10);
        //printf("%u\n", column_id);
        entry->values[column_id].discrete = value;
      }
      column_id++;
      // Obtain next token.
      token = strtok(NULL, ",");
    }
    // Verify that the feature count matches the value count.
    if (column_id != dataset->header->feature_count) {
      if (DEBUG) {
        printf("[%s : %d] %s %u %u %u\n", __FILE__, __LINE__, "Unable to parse data file:", count, column_id, dataset->header->feature_count);
      }
      free(names_path);
      free(data_path);
      dataset_dispose(dataset, true, true);
      return NULL;
    }
    if (DEBUG) {
      printf("[%s : %d] %s %u\n", __FILE__, __LINE__, "Loaded entry #", count);
    }
    previous = entry;
    count++;
  }
  dataset->tail = previous;
  // Close the file after parsing.
  fclose(fp);
  free(line);
  dataset->entry_count = count;
  // Print the final entry count for debug purposes.
  if (DEBUG) {
    printf("[%s : %d] %s %u\n", __FILE__, __LINE__, "Final entry count:", dataset->entry_count);
  }
  free(names_path);
  free(data_path);
  return dataset;
}
