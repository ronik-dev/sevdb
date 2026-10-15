#include <criterion/criterion.h>
#include <stdio.h>
#include <stdlib.h>
#include "sevdb/sevdb.h"

// Helper to mutate a file for corruption testing
static void mutate_file(const char* path, long offset, uint8_t bad_byte) {
    FILE *f = fopen(path, "r+b");
    cr_assert_not_null(f, "Failed to open file for mutation");
    fseek(f, offset, SEEK_SET);
    fwrite(&bad_byte, 1, 1, f);
    fclose(f);
}

// Helper to truncate a file
static void truncate_file(const char* path, long new_size) {
    FILE *f = fopen(path, "r+b"); // Open for read/update
    cr_assert_not_null(f, "Failed to open file for truncation");
    // POSIX truncate, or just copy to a new file to be portable in tests
    uint8_t *buffer = malloc(new_size);
    fread(buffer, 1, new_size, f);
    fclose(f);
    
    f = fopen(path, "wb");
    fwrite(buffer, 1, new_size, f);
    fclose(f);
    free(buffer);
}

Test(persistence, should_serialize_and_deserialize_successfully) {
    const char *path = "test_db_happy_path.sevdb";
    sevdb_database *db = NULL;
    sevdb_db_create(5, &db);

    float components[2] = {1.5f, 2.5f};
    sevdb_vector *v = NULL;
    sevdb_vector_create(42, 2, components, &v);
    sevdb_db_push_vector(db, v);

    cr_assert(sevdb_db_serialize(db, path), "Serialization failed");
    sevdb_db_destroy(db);

    sevdb_database *loaded_db = sevdb_db_deserialize(path);
    cr_assert_not_null(loaded_db, "Deserialization returned NULL on valid file");
    
    cr_assert_eq(sevdb_db_get_count(loaded_db), 1);
    sevdb_vector *loaded_v = sevdb_db_get_vector_by_id(loaded_db, 42);
    cr_assert_not_null(loaded_v);
    cr_assert_float_eq(sevdb_vector_get_components(loaded_v)[0], 1.5f, 0.0001f);

    sevdb_db_destroy(loaded_db);
    remove(path);
}

Test(persistence, should_reject_invalid_magic_number) {
    const char *path = "test_db_bad_magic.sevdb";
    sevdb_database *db = NULL;
    sevdb_db_create(5, &db);
    sevdb_db_serialize(db, path);
    sevdb_db_destroy(db);

    // Mutate the first byte of the "SEVDB" magic string[cite: 5]
    mutate_file(path, 0, 'X');

    sevdb_database *loaded_db = sevdb_db_deserialize(path);
    cr_assert_null(loaded_db, "Deserializer should reject invalid magic numbers");

    remove(path);
}

Test(persistence, should_reject_unsupported_version) {
    const char *path = "test_db_bad_version.sevdb";
    sevdb_database *db = NULL;
    sevdb_db_create(5, &db);
    sevdb_db_serialize(db, path);
    sevdb_db_destroy(db);

    // Version is a uint32_t located right after the 5-byte magic string[cite: 5]
    mutate_file(path, 5, 0x02); // Change version 1 to version 2

    sevdb_database *loaded_db = sevdb_db_deserialize(path);
    cr_assert_null(loaded_db, "Deserializer should reject unsupported versions");

    remove(path);
}

Test(persistence, should_reject_truncated_file) {
    const char *path = "test_db_truncated.sevdb";
    sevdb_database *db = NULL;
    sevdb_db_create(5, &db);
    
    float components[2] = {1.0f, 2.0f};
    sevdb_vector *v = NULL;
    sevdb_vector_create(1, 2, components, &v);
    sevdb_db_push_vector(db, v);
    sevdb_db_serialize(db, path);
    sevdb_db_destroy(db);

    // Truncate the file to only 20 bytes (cuts off halfway through the vector payload)
    truncate_file(path, 20);

    sevdb_database *loaded_db = sevdb_db_deserialize(path);
    cr_assert_null(loaded_db, "Deserializer should reject truncated files safely without crashing");

    remove(path);
}

Test(persistence, should_reject_corrupted_checksum) {
    const char *path = "test_db_bad_crc.sevdb";
    sevdb_database *db = NULL;
    sevdb_db_create(5, &db);
    sevdb_db_serialize(db, path);
    sevdb_db_destroy(db);

    // Determine file size to mutate the final CRC32 bytes[cite: 5]
    FILE *f = fopen(path, "r+b");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fclose(f);

    // Corrupt the very last byte of the footer
    mutate_file(path, size - 1, 0xFF);

    sevdb_database *loaded_db = sevdb_db_deserialize(path);
    cr_assert_null(loaded_db, "Deserializer should reject files with a mismatched CRC32 footer");

    remove(path);
}
