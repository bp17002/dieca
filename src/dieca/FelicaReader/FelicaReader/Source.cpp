// sample.cpp -*-c++-*-

#include <cstdio>
#include <cstdlib>

#include "felica.h"

int main(void);
void error_routine(void);
void print_vector(const char* title, unsigned char* vector, int length);

int main(void)
{
	if (!initialize_library()) {
		fprintf(stderr, "Can't initialize library.\n");
		return EXIT_FAILURE;
	}
	printf("Complete initialize_library\r\n");

	if (!open_reader_writer_auto()) {
		fprintf(stderr, "Can't open reader writer.\n");
		return EXIT_FAILURE;
	}
	printf("Complete open_reader_writer_auto\r\n");

	structure_polling polling;
	unsigned char system_code[2] = { 0xFF, 0xFF };//{ 0x00, 0x00 } -> { 0xFF, 0xFF }
	polling.system_code = system_code;
	polling.time_slot = 0x00;

	unsigned char number_of_cards = 0;

	structure_card_information card_information;
	unsigned char card_idm[8];
	unsigned char card_pmm[8];
	card_information.card_idm = card_idm;
	card_information.card_pmm = card_pmm;


	unsigned long timeout;
	timeout = 100;
	set_polling_timeout(timeout);
	get_polling_timeout(&timeout);
	printf("timeout: %lu\r\n", timeout);

	while (!polling_and_get_card_information(&polling, &number_of_cards, &card_information)) {
		//fprintf(stderr, "Can't find FeliCa.\n");
		//return EXIT_FAILURE;
	}

	fprintf(stdout, "number of cards: %d\n", number_of_cards);

	print_vector("[readIDm]:", card_idm, sizeof(card_idm));
	print_vector("[readPMm]:", card_pmm, sizeof(card_pmm));

	if (!close_reader_writer()) {
		fprintf(stderr, "Can't close reader writer.\n");
		return EXIT_FAILURE;
	}

	if (!dispose_library()) {
		fprintf(stderr, "Can't dispose library.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void error_routine(void)
{
	enumernation_felica_error_type felica_error_type;
	enumernation_rw_error_type rw_error_type;
	get_last_error_types(&felica_error_type, &rw_error_type);
	printf("felica_error_type: %d\n", felica_error_type);
	printf("rw_error_type: %d\n", rw_error_type);

	close_reader_writer();
	dispose_library();
}

void print_vector(const char* title, unsigned char* vector, int length)
{
	if (title != NULL) {
		fprintf(stdout, "%s ", title);
	}

	int i;
	for (i = 0; i < length - 1; i++) {
		fprintf(stdout, "%02x ", vector[i]);
	}
	fprintf(stdout, "%02x", vector[length - 1]);
	fprintf(stdout, "\n");
}
