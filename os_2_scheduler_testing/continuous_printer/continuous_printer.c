#include <stdio.h>

void stupid_calculation() {
	int i;
	for(i=0; i<42424242; ++i) {
		float thing = 3.1415;
		double other = (thing++/5.4)+(thing*thing*4.6798454);
		other = (other + thing)/(2.4*thing)-other;
	}
}

int main(int argc, char** argv) {
	if (argc!=2) {
		printf("invalid arguments\n",argc);
		return -1;
	}

	int i;
	for(i=0; i<20; ++i){
		stupid_calculation();
		printf("%s\n",argv[1]);
	}

	return 0;
}
