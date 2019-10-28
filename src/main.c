#include "dataset.h"
#include "information.h"
#include "benchmarking.h"
#include "decision_tree.h"
#include "random_forest.h"
#include <time.h>
#include <errno.h>
#include <sys/resource.h>

int main(int argc, char* argv[]) {
  srand(time(NULL));
  RandomForest *test = random_forest_generate_full("rust-train");
  Dataset *data = dataset_load_from_disk("rust-train");
  Dataset *test_data = dataset_load_from_disk("rust-test");

  random_forest_test(test, test_data);
  BooleanMask *mask = boolean_mask_create();
  DecisionTree *tree = decision_tree_create(data,mask,NULL);
  decision_tree_train(tree);
  decision_tree_test(tree, test_data);
  //int test_prediction = decision_tree_predict(tree, data->head);
  //printf("%d\n", test_prediction);
  //decision_tree_output_graph(tree);
  //decision_tree_print(tree,0);
  /*
  benchmarking_init();
  unsigned int dummy = 0;
  double dummy2 = 0.0;
  for (unsigned int i = 0; i < 100; i++) {
    Dataset *data = dataset_load_from_disk("rust");
    printf("%u\n", i);
    benchmarking_probe_a("rust");
    information_find_best_split(data, &dummy, &dummy2);
    benchmarking_probe_b("rust");
    dataset_dispose(data, true, true);
    Dataset *data2 = dataset_load_from_disk("rust-test");
    printf("%u\n", i);
    benchmarking_probe_a("rust-test");
    information_find_best_split(data2, &dummy, &dummy2);
    benchmarking_probe_b("rust-test");
    dataset_dispose(data2, true, true);
    Dataset *data3 = dataset_load_from_disk("rust-test");
    printf("%u\n", i);
    benchmarking_probe_a("rust-train");
    information_find_best_split(data3, &dummy, &dummy2);
    benchmarking_probe_b("rust-train");
    dataset_dispose(data3, true, true);
  }
  benchmarking_end();
  */
  //double test_split = information_gain_on_split(test, 1, 7.765);
  //information_print_split_data(test, 1);
  //unsigned int feature = 0;
  //double split_value = 0;
  //information_find_best_split(test, &feature, &split_value);
  //printf("%u, %f\n", feature, split_value);

}
