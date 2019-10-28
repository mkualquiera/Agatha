
#if INTERFACE

#include <stdio.h>
#include "dataset.h"
#include "boolean_mask.h"
#include "decision_tree.h"

typedef struct RandomForest {
  DecisionTree **trees;
  unsigned int *weights;
  unsigned int tree_count;
} RandomForest;

#endif


#include "random_forest.h"

void random_forest_test(RandomForest *forest, Dataset *dataset) {
  DatasetEntry *focus = dataset->head;
  unsigned int success = 0;
  unsigned int total = 0;
  printf("Testing forest: ");
  while(focus != NULL) {
    int test = random_forest_predict(forest, focus);
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

int random_forest_predict(RandomForest *forest, DatasetEntry *entry) {
  unsigned int poss_count = forest->trees[0]->dataset->header->label->discrete_possibility_count;
  unsigned int *counts = calloc(poss_count - 1,sizeof(unsigned int));
  unsigned int max_count = 0;
  unsigned int max_index = 0;
  for (unsigned int i = 0; i < forest->tree_count; i++) {
    int prediction = decision_tree_predict(forest->trees[i], entry);
    unsigned int index = 0;
    for (unsigned int j = 0; j < poss_count; j++) {
      if (forest->trees[0]->dataset->header->label->discrete_possibles[j] == prediction) {
        index = j;
      }
    }
    counts[index] += forest->weights[i];
    if (counts[index] > max_count) {
      max_count = counts[index];
      max_index = index;
    }
  }
  free(counts);
  return forest->trees[0]->dataset->header->label->discrete_possibles[max_index];
}

RandomForest *random_forest_generate_full (char *path) {
  Dataset *reference_dataset = dataset_load_from_disk(path);
  unsigned int true_features = reference_dataset->header->feature_count - 1;
  dataset_dispose(reference_dataset, true, true);
  RandomForest *forest = malloc(sizeof(RandomForest));
  unsigned int tree_count = (1 << true_features) - 2;
  forest->tree_count = tree_count;
  forest->trees = malloc(sizeof(DecisionTree*) * tree_count);
  forest->weights = malloc(sizeof(unsigned int) * tree_count);
  for (unsigned int i = 0; i <= tree_count; i++) {
    BooleanMask *mask = boolean_mask_create();
    unsigned int weight = 0;
    printf("Training tree ");
    for (unsigned int j = 0; j<true_features; j++) {
      unsigned int bit = (i & (1 << j)) >> j;
      weight += (1-bit);
      printf("%u", bit);
      boolean_mask_set(mask, j, bit);
    }
    weight *= (weight)/2;
    printf("\n");
    Dataset *dataset = dataset_load_from_disk(path);
    forest->trees[i] = decision_tree_create(dataset, mask, NULL);
    forest->weights[i] = weight;
    decision_tree_train(forest->trees[i]);
  }
  return forest;
}
