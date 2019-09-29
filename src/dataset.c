#if INTERFACE

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
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

void dataset_feature_dispose(DatasetFeature* target) {
  free(target->name);
  free(target->discrete_possibles);
  free(target);
}

void dataset_header_dispose(DatasetHeader* header) {
  for(unsigned int i = 0; i < header->feature_count; i++) {
    dataset_feature_dispose(header->features[i]);
  }
  free(header);
}

DatasetHeader* dataset_header_create() {
  DatasetHeader *result = malloc(sizeof(*result));
  result->max_feature_count = FEATURE_COUNT_STEP;
  result->features = malloc(sizeof(DatasetFeature*) * result->max_feature_count);
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
    DatasetFeature** newFeatures = malloc(sizeof(DatasetFeature*) * result->max_feature_count);
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
  char *data_path = malloc(sizeof(char*) * data_path_length);
  char *names_path = malloc(sizeof(char*) * names_path_length);
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
    DatasetFeature *feature = malloc(sizeof(*feature));
    unsigned char column_id = 0;
    // Tokenize the line for parsing.
    char *token = strtok(line, " ");
    while (token != NULL) {
      if (column_id == 0) {
        // Copy the name into the feature.
        feature->name = malloc(sizeof(char) * strlen(token) + 1);
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
            unsigned int pos_count = strol(token, &end, 10);
            feature->discrete_possibility_count = pos_count;
            feature->discrete_possibles = malloc(sizeof(int) * pos_count);
          }
          if (column_id >= 4) {
            // Add the discrete possibilities to the feature.
            unsigned int index = columnId - 4;
            // Make sure the possibility count is not overflowing.
            if (index < feature->discrete_possibles) {
              char* end;
              feature->discrete_possibles[index] = strol(token, &end, 10);
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
  if (!found_label | (dataset->header->feature_count < 2)) {
    if (DEBUG) {
      printf("[%s : %d] %s\n", __FILE__, __LINE__, "The dataset must have at least two features and a label.");
    }
    free(names_path);
    free(data_path);
    dataset_header_dispose(header);
    return NULL;
  }
  return NULL;
}
