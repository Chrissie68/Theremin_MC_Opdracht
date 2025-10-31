#include <unity.h>
#include "MediaanFilter.c"

void test_filter_function(void) {
    // test: controleer of de filtergrootte correct wordt aangepast
    filterSize = 5;
    buffer[0].age = 0; buffer[0].value = 10;
    buffer[1].age = 1; buffer[1].value = 20;
    buffer[2].age = 2; buffer[2].value = 30;
    buffer[3].age = 3; buffer[3].value = 40;
    buffer[4].age = 4; buffer[4].value = 50;

    uint16_t result = mediaan_filter(30); // Voeg nieuwe waarde toe
    TEST_ASSERT_EQUAL_UINT16(30, result); // Verwachte mediaan is 30
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_filter_function);
    return UNITY_END();
}