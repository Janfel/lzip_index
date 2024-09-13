#include "lzip_index.h"

#include <assert.h>
#include <stdlib.h>


int lzip_index_prepend(struct lzip_index *index, struct lzip_index_member const *member)
{
	assert(index);
	assert(member);

	struct lzip_index_member *new_members =
		realloc(index->members, (index->member_count + 1) * sizeof(index->members[0]));
	if (!new_members) return 1;

	index->members = new_members;
	index->members[index->member_count++] = *member;
	return 0;
}


void lzip_index_finalize(struct lzip_index *index)
{
	assert(index);

	uint64_t combined_data_size = 0;
	uint64_t indexable_data_size = 0;

	for (size_t j = 0; j < index->member_count; ++j) {
		size_t i = index->member_count - j - 1;
		struct lzip_index_member *member = &index->members[i];

		member->data_offset = combined_data_size;
		combined_data_size += member->data_size;

		if (j == 0) {
			indexable_data_size = member->data_size;
			continue;
		}

		if (i == 0 || indexable_data_size == 0) {
			continue;
		}

		if (member->data_size != indexable_data_size) {
			indexable_data_size = 0;
		}
	}

	index->combined_data_size = combined_data_size;
	index->indexable_data_size = indexable_data_size;
}

/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) */
static int comparator(void const *key, void const *element)
{
	uint64_t const *data_offset_ptr = key;
	uint64_t data_offset = *data_offset_ptr;

	struct lzip_index_member const *member = element;

	if (data_offset < member->data_offset) return +1;
	if (data_offset < member->data_offset + member->data_size) return 0;
	return -1;
}

struct lzip_index_member const *
lzip_index_search(struct lzip_index const *index, uint64_t data_offset)
{
	if (!index) return NULL;
	if (data_offset > index->combined_data_size) return NULL;

	if (index->indexable_data_size) {
		size_t member_index = data_offset / index->indexable_data_size;
		return &index->members[index->member_count - member_index];
	}

	return bsearch(
		&data_offset, index->members, index->member_count, sizeof(index->members[0]), comparator);
}


void lzip_index_destroy(struct lzip_index *index)
{
	if (!index) return;
	free(index->members);
	*index = (struct lzip_index){};
}
