#if INTERFACE
#include "dataset.h"
#include <math.h>
#endif

#include "information.h"

unsigned int* information_dataset_count(Dataset *dataset, bool limited, unsigned int feature_index, bool greater_than, double split_value) {
  if (!limited) {
    if (dataset->has_cached_counts) {
      return dataset->counts;
    }
  }
  unsigned int label_index = dataset->header->label_index;
  DatasetFeature *label = dataset->header->label;
  unsigned int *result = calloc((label->discrete_possibility_count + 1), sizeof(unsigned int));
  DatasetEntry *focus = dataset->head;
  while ((focus != dataset->tail) & (focus != NULL)) {
    bool shouldCount = true;
    if (limited) {
      bool passes_limit = focus->values[feature_index].continous < split_value;
      if (greater_than) {
        passes_limit = !passes_limit;
      }
      shouldCount = passes_limit;
    }
    if (shouldCount) {
      for (unsigned int i = 0; i < label->discrete_possibility_count; i++) {
        if (focus->values[label_index].discrete == label->discrete_possibles[i]) {
          result[i+1]++;
          break;
        }
      }
      result[0]++;
    }
    focus = focus->next;
  }
  if (!limited) {
    if(!dataset->has_cached_counts) {
      dataset->counts = result;
      dataset->has_cached_counts = true;
    }
  }
  return result;
}

double information_entropy(unsigned int *counts, unsigned int counts_count) {
  double accumulation = 0;
  for (unsigned int i = 0; i < counts_count; i++) {
    unsigned int index = i+1;
    if (counts[index] == 0) {
      continue;
    }
    double fraction = (double)counts[index] / (double)counts[0];
    accumulation += fraction * (log(fraction)/log(2));
  }
  return -accumulation;
}

double information_gain_on_split(Dataset *dataset, unsigned int feature_index, double split_value) {
  unsigned int* parent_count = information_dataset_count(dataset, false, 0, false, 0.0);
  unsigned int* left_count = information_dataset_count(dataset, true, feature_index, false, split_value);
  unsigned int* right_count = information_dataset_count(dataset, true, feature_index, true, split_value);
  double parent_entropy = information_entropy(parent_count, dataset->header->label->discrete_possibility_count);
  double left_entropy = information_entropy(left_count, dataset->header->label->discrete_possibility_count);
  double right_entropy = information_entropy(right_count, dataset->header->label->discrete_possibility_count);
  double final_entropy = (((double)left_count[0] / (double)parent_count[0]) * left_entropy) + (((double)right_count[0] / (double)parent_count[0]) * right_entropy);
  return parent_entropy - final_entropy;
}

void information_find_best_split(Dataset *dataset, unsigned int* feature_index, double* split_value) {
  double best_gain = 0.0;
  unsigned int best_feature_index = 0;
  double best_split_value = 0.0;
  for (unsigned int feature_index = 0; feature_index < dataset->header->feature_count; feature_index++) {
    //information_save_split_data(dataset, feature_index);
    DatasetFeature *feature = dataset->header->features[feature_index];
    if (feature->type == DISCRETE) {
      continue;
    }
    double step_size = pow(10, -feature->continuous_precision-1) * 5;
    for(double split = feature->continuous_lower_boundary; split < feature->continuous_upper_boundary; split += step_size) {
      double gain = information_gain_on_split(dataset, feature_index, split);
      if (gain > best_gain) {
        best_gain = gain;
        best_feature_index = feature_index;
        best_split_value = split;
      }
    }
  }
  *feature_index = best_feature_index;
  *split_value = best_split_value;
}

void information_save_split_data(Dataset *dataset, unsigned int feature_index) {
  DatasetFeature *feature = dataset->header->features[feature_index];
  FILE *data_file = fopen(feature->name, "w");
  double step_size = pow(10, -feature->continuous_precision-1) * 5;
  for(double split = feature->continuous_lower_boundary; split < feature->continuous_upper_boundary; split += step_size) {
    double gain = information_gain_on_split(dataset, feature_index, split);
    fprintf(data_file, "%f %f\n", split, gain);
  }
  fclose(data_file);
}
