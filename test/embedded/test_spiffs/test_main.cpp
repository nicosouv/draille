#include <Arduino.h>
#include <unity.h>
#include <SPIFFS.h>

// Integration tests — requires ESP32 with SPIFFS partition

void setUp(void) {}
void tearDown(void) {}

void test_spiffs_mounts() {
    bool ok = SPIFFS.begin(true);
    TEST_ASSERT_TRUE(ok);
}

void test_write_and_read_file() {
    const char* path    = "/test_rw.txt";
    const char* content = "draille-test-123";

    File w = SPIFFS.open(path, FILE_WRITE);
    TEST_ASSERT_TRUE(w);
    w.print(content);
    w.close();

    File r = SPIFFS.open(path, FILE_READ);
    TEST_ASSERT_TRUE(r);

    char buf[32] = {};
    size_t n = r.readBytes(buf, sizeof(buf) - 1);
    r.close();

    TEST_ASSERT_GREATER_THAN(0u, n);
    TEST_ASSERT_EQUAL_STRING(content, buf);

    SPIFFS.remove(path);
}

void test_file_exists_after_write() {
    const char* path = "/test_exists.txt";

    File f = SPIFFS.open(path, FILE_WRITE);
    f.print("x");
    f.close();

    TEST_ASSERT_TRUE(SPIFFS.exists(path));

    SPIFFS.remove(path);
    TEST_ASSERT_FALSE(SPIFFS.exists(path));
}

void test_mkdir_tracks() {
    if (!SPIFFS.exists("/tracks")) {
        SPIFFS.mkdir("/tracks");
    }
    TEST_ASSERT_TRUE(SPIFFS.exists("/tracks"));
}

void test_gpx_file_write() {
    const char* path = "/tracks/test_session.gpx";
    const char* header =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<gpx version=\"1.1\" creator=\"Draille\">\n"
        "  <trk><trkseg>\n";
    const char* trkpt =
        "    <trkpt lat=\"44.2800000\" lon=\"4.7200000\">"
        "<ele>85.0</ele></trkpt>\n";
    const char* footer =
        "  </trkseg></trk>\n</gpx>\n";

    File f = SPIFFS.open(path, FILE_WRITE);
    TEST_ASSERT_TRUE(f);
    f.print(header);
    f.print(trkpt);
    f.print(footer);
    f.close();

    TEST_ASSERT_TRUE(SPIFFS.exists(path));
    TEST_ASSERT_GREATER_THAN(0u, (uint32_t)SPIFFS.open(path).size());

    SPIFFS.remove(path);
}

void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_spiffs_mounts);
    RUN_TEST(test_write_and_read_file);
    RUN_TEST(test_file_exists_after_write);
    RUN_TEST(test_mkdir_tracks);
    RUN_TEST(test_gpx_file_write);
    UNITY_END();
}

void loop() {}
