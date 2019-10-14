#if INTERFACE

#include <stdlib.h>
#include "dataset.h"
#include <stdbool.h>
#include "information.h"

typedef struct DecisionTree {
  Dataset *dataset;
  unsigned int feature_mask;
  unsigned int feature_index;
  double split_value;
  DecisionTree *parent;
  DecisionTree *left;
  DecisionTree *right;
  bool is_leaf;
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

void decision_tree_output_graph (DecisionTree *decision_tree) {
  if (decision_tree->left->is_leaf) {
    printf("\"%s<%f\"->\"Rust probability: %f\"\n",decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value,decision_tree->left->dataset->header->features[decision_tree->left->feature_index]->name, decision_tree->dataset->header->features[decision_tree->feature_index]->name, (double)decision_tree->left->dataset->counts[2]/(double)decision_tree->left->dataset->entry_count);
  } else {
    printf("\"%s<%f\"->\"%s<%f\"\n", decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value,decision_tree->left->dataset->header->features[decision_tree->left->feature_index]->name, decision_tree->left->split_value);
    printf("\"%s<%f\"->\"%s>=%f\"\n", decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value,decision_tree->left->dataset->header->features[decision_tree->left->feature_index]->name, decision_tree->left->split_value);
    decision_tree_output_graph(decision_tree->left);
  }
  if (decision_tree->right->is_leaf) {
    printf("\"%s>=%f\"->\"Rust probability: %f\"\n",decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value,decision_tree->left->dataset->header->features[decision_tree->left->feature_index]->name, decision_tree->dataset->header->features[decision_tree->feature_index]->name, (double)decision_tree->right->dataset->counts[2]/(double)decision_tree->right->dataset->entry_count);
  } else {
    printf("\"%s>=%f\"->\"%s<%f\"\n", decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value,decision_tree->right->dataset->header->features[decision_tree->right->feature_index]->name, decision_tree->right->split_value);
    printf("\"%s>=%f\"->\"%s>=%f\"\n", decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value,decision_tree->right->dataset->header->features[decision_tree->right->feature_index]->name, decision_tree->right->split_value);
    decision_tree_output_graph(decision_tree->right);
  }
}

void decision_tree_print(DecisionTree *decision_tree, unsigned int depth) {
  if (decision_tree->is_leaf) {
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("Rust probability: %f\n", (double)decision_tree->dataset->counts[2]/(double)decision_tree->dataset->entry_count);
  } else {
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("if (%s < %f)\n", decision_tree->dataset->header->features[decision_tree->feature_index]->name, decision_tree->split_value);
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("{\n");
    decision_tree_print(decision_tree->left, depth + 1);
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("}\n");
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("else\n");
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("{\n");
    decision_tree_print(decision_tree->right, depth + 1);
    for (unsigned int i = 0; i < depth; i++) {
      printf(" ");
    }
    printf("}\n");
  }
}

void decision_tree_train(DecisionTree *decision_tree) {
  printf("Training on %u entries.\n", decision_tree->dataset->entry_count);
  unsigned int feature_index;
  double split_value;
  printf("Trying to find best split...\n");
  double gain = information_find_best_split(decision_tree->dataset, &feature_index,&split_value);
  if (gain > 0.0001) {
    printf("Found best split at %u : %f with %f information gain\n", feature_index, split_value, gain);
    decision_tree->feature_index = feature_index;
    decision_tree->split_value = split_value;
    Dataset *left_dataset;
    Dataset *right_dataset;
    dataset_split(decision_tree->dataset, decision_tree->feature_index,  decision_tree->split_value,&left_dataset,&right_dataset);
    decision_tree->left = decision_tree_create(left_dataset, decision_tree->feature_mask, decision_tree);
    decision_tree->right = decision_tree_create(right_dataset, decision_tree->feature_mask, decision_tree);
    decision_tree_train(decision_tree->left);
    decision_tree_train(decision_tree->right);
  } else {
    decision_tree->is_leaf = true;
    printf("Unable to find a good enough split.\n");
  }
}
