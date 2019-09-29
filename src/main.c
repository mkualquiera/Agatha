#include "dataset.h"
#include "information.h"

int main(int argc, char* argv[]) {
  Dataset *test = dataset_load_from_disk("rust");
  //double test_split = information_gain_on_split(test, 1, 7.765);
  //information_print_split_data(test, 1);
  unsigned int feature = 0;
  double split_value = 0;
  information_find_best_split(test, &feature, &split_value);
  printf("%u, %f\n", feature, split_value);

}
