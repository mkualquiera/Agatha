#if INTERFACE

#include <stdlib.h>
#include "dataset.h"
#include <stdbool.h>
#include "information.h"
#include "boolean_mask.h"
#include "utils.h"

typedef struct DecisionTree {
  Dataset *dataset;
  BooleanMask *feature_mask;
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
DecisionTree* decision_tree_create (Dataset *dataset, BooleanMask *feature_mask, DecisionTree *parent) {
  DecisionTree *result = calloc(1, sizeof(DecisionTree));
  result->dataset = dataset;
  result->feature_mask = feature_mask;
  result->parent = parent;
  return result;
}

void decision_tree_test(DecisionTree *tree, Dataset *dataset) {
  DatasetEntry *focus = dataset->head;
  unsigned int success = 0;
  unsigned int total = 0;
  printf("Testing tree: ");
  while(focus != NULL) {
    int test = decision_tree_predict(tree, focus);
    int expected = focus->values[dataset->header->label_index].discrete;
    if (test == expected) {
      success++;
    }
    if (total%40 == 0) {
      printf("\n");
    }
    printf("%u", test==expected);
    total++;
    focus = focus->next;
  }
  double accuracy = (double)success / (double)total;
  printf("\nPassed %u out of %u: %f accuracy.\n", success, total, accuracy);
}

int decision_tree_predict(DecisionTree *tree, DatasetEntry *entry) {
  if(tree->is_leaf) {
    unsigned int poss_count = tree->dataset->header->label->discrete_possibility_count;
    unsigned int *cummulative_counts = malloc(sizeof(unsigned int) * poss_count - 1);
    int decision = tree->dataset->header->label->discrete_possibles[0];
    for (unsigned int i = 0; i < poss_count; i++) {
      if(i == 0) {
        cummulative_counts[i] = tree->dataset->counts[i+1];
      } else {
        cummulative_counts[i] = cummulative_counts[i-1] + tree->dataset->counts[i+1];
      }
    }
    unsigned int random_gen = randint(tree->dataset->counts[0]);
    for (unsigned int i = 0; i < poss_count; i++) {
      unsigned int lower_boundary = 0;
      if (i != 0) {
        lower_boundary = cummulative_counts[i-1];
      }
      unsigned int upper_boundary = cummulative_counts[i];
      if((random_gen >= lower_boundary) & (random_gen < upper_boundary)) {
        free(cummulative_counts);
        return tree->dataset->header->label->discrete_possibles[i];
      }
    }
  } else {
    if (entry->values[tree->feature_index].continous < tree->split_value) {
      return decision_tree_predict(tree->left, entry);
    } else {
      return decision_tree_predict(tree->right, entry);
    }
  }
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
  //printf("Training on %u entries.\n", decision_tree->dataset->entry_count);
  unsigned int feature_index;
  double split_value;
  //printf("Trying to find best split...\n");
  double gain = information_find_best_split(decision_tree->dataset, &feature_index,&split_value,decision_tree->feature_mask);
  if (gain > 0.0001) {
    //printf("Found best split at %u : %f with %f information gain\n", feature_index, split_value, gain);
    decision_tree->feature_index = feature_index;
    decision_tree->split_value = split_value;
    BooleanMask *new_mask = boolean_mask_copy(decision_tree->feature_mask);
    boolean_mask_set(new_mask, feature_index, 1);
    decision_tree->feature_mask = new_mask;
    Dataset *left_dataset;
    Dataset *right_dataset;
    dataset_split(decision_tree->dataset, decision_tree->feature_index,  decision_tree->split_value,&left_dataset,&right_dataset);
    decision_tree->left = decision_tree_create(left_dataset, decision_tree->feature_mask, decision_tree);
    decision_tree->right = decision_tree_create(right_dataset, decision_tree->feature_mask, decision_tree);
    decision_tree_train(decision_tree->left);
    decision_tree_train(decision_tree->right);
  } else {
    decision_tree->is_leaf = true;
    information_dataset_count(decision_tree->dataset, false, 0, false, 0.0);
    //printf("Unable to find a good enough split.\n");
  }
}
