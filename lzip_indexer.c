#include "lzip_index.h"

#include <assert.h>
#include <endian.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define MINIMUM_LZIP_MEMBER_SIZE 27

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "No input file\n");
		return EXIT_FAILURE;
	}

	FILE *file = fopen(argv[1], "rb");
	if (!file) {
		perror("Failed to open input file");
		return EXIT_FAILURE;
	}

	if (fseek(file, 0, SEEK_END)) {
		perror("Failed to seek to end of file");
		return EXIT_FAILURE;
	}

	long file_size = ftell(file);
	if (file_size == -1) {
		perror("Failed to get file size");
		return EXIT_FAILURE;
	}

	struct lzip_index index = {};
	struct lzip_index_member member = {};

	uint64_t size_data[2];
	long size_data_bytes = sizeof(size_data);

	while (file_size) {
		if (fseek(file, -size_data_bytes, SEEK_CUR)) {
			perror("Failed to seek backwards at member boundary");
			return EXIT_FAILURE;
		}

		if (fread(size_data, sizeof(size_data), 1, file) < 1) {
			fprintf(stderr, "Failed to read member size data\n");
			return EXIT_FAILURE;
		}

		uint64_t data_size = le64toh(size_data[0]);
		uint64_t member_size = le64toh(size_data[1]);

		if (member_size < MINIMUM_LZIP_MEMBER_SIZE) {
			fprintf(stderr, "Member size is impossibly small");
			return EXIT_FAILURE;
		}

		if (member_size > file_size) {
			fprintf(stderr, "Member size exceeds remaining file size");
			return EXIT_FAILURE;
		}

		if (fseek(file, -((long) member_size), SEEK_CUR)) {
			perror("Failed to seek backwards to beginning of member");
			return EXIT_FAILURE;
		}

		long member_offset = ftell(file);
		if (member_offset == -1) {
			perror("Failed to get member offset");
			return EXIT_FAILURE;
		}

		member.data_offset = 0;
		member.data_size = data_size;
		member.member_offset = member_offset;
		member.member_size = member_size;

		if (lzip_index_prepend(&index, &member)) {
			perror("Allocation failure while growing the index");
			return EXIT_FAILURE;
		}

		file_size = member_offset;
	}

	lzip_index_finalize(&index);

	printf("INDEX:\n");
	printf("\n");
	printf("File Size  : %lu\n", index.combined_data_size);
	printf("Block Size : %lu\n", index.indexable_data_size);
	printf("Members    : %zu\n", index.member_count);
	printf("\n");
	printf("member      data_pos      data_size     member_pos    member_size\n");

	for (size_t i = 0; i < index.member_count; ++i) {
		size_t member_index = index.member_count - i - 1;
		struct lzip_index_member const *member = &index.members[member_index];
		printf(
			"%5zu %14lu %14lu %14lu %14lu\n",
			i + 1,
			member->data_offset,
			member->data_size,
			member->member_offset,
			member->member_size);
	}

    lzip_index_destroy(&index);
	fclose(file);
	return EXIT_SUCCESS;
}
