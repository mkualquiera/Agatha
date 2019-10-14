#if INTERFACE

#include <stdlib.h>
#include "dataset.h"
#include "information.h"

typedef struct DecisionTree {
  Dataset *dataset;
  unsigned int feature_mask;
  unsigned int feature_index;
  double split_value;
  DecisionTree *parent;
  DecisionTree *left;
  DecisionTree *right;
} DecisionTree;

#endif

#include "decision_tree.h"

// Create a new decision tree from a dataset.
DecisionTree* decision_tree_create (Dataset *dataset, unsigned int feature_mask, DecisionTree *parent) {
  DecisionTree *result = calloc(1, sizeof(DecisionTree));
  result->dataset = dataset;
  result->feature_mask = feature_mask;
  result->parent = parent;
  return result;
}

void decision_tree_optimize(DecisionTree *decision_tree) {
  information_find_best_split(decision_tree->dataset, &decision_tree->feature_index,&decision_tree->split_value);
  Dataset *left_dataset;
  Dataset *right_dataset;
  dataset_split(decision_tree->dataset, decision_tree->feature_index, decision_tree->split_value,&left_dataset,&right_dataset);
}
