#include "unity.h"
#include "defs.h"
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

void run_command(char *command, char *output, size_t output_size);

static struct sysinfo fake_si;
static int            fake_sysinfo_ret;
static long           fake_ncpu;

static char   fake_output[1024];
static size_t fake_output_len;
static int    fake_popen_fails;
static char   last_command[256];
static int    pclose_count;

static char fake_fp_storage;
#define FAKE_FP ((FILE *)&fake_fp_storage)


// some homegrown mocks for system calls
// use --wrap= to override the calls in measurement.c

int __wrap_sysinfo(struct sysinfo *info)
{
    if (fake_sysinfo_ret == 0) {
        *info = fake_si;
    }
    return fake_sysinfo_ret;
}

long __wrap_sysconf(int name)
{
    (void)name;
    return fake_ncpu;
}

FILE *__wrap_popen(const char *command, const char *type)
{
    (void)type;
    snprintf(last_command, sizeof(last_command), "%s", command);
    return fake_popen_fails ? NULL : FAKE_FP;
}

size_t __wrap_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    (void)stream;
    size_t want = size * nmemb;
    size_t n = fake_output_len < want ? fake_output_len : want;
    memcpy(ptr, fake_output, n);
    return size ? n / size : 0;
}

int __wrap_pclose(FILE *stream)
{
    (void)stream;
    pclose_count++;
    return 0;
}

static void reset_fakes(void)
{
    memset(&fake_si, 0, sizeof(fake_si));
    fake_si.mem_unit = 1;
    fake_sysinfo_ret = 0;
    fake_ncpu = 1;
    memset(fake_output, 0, sizeof(fake_output));
    fake_output_len = 0;
    fake_popen_fails = 0;
    last_command[0] = '\0';
    pclose_count = 0;
}

static void set_output(const char *s)
{
    snprintf(fake_output, sizeof(fake_output), "%s", s);
    fake_output_len = strlen(s);
}

static void set_load1(double load)
{
    fake_si.loads[0] = (unsigned long)(load * (1 << SI_LOAD_SHIFT));
}

static int cpu_for(double load, long ncpu)
{
    int cpu = -1, mem = -1;
    set_load1(load);
    fake_si.totalram = 1000;
    fake_ncpu = ncpu;
    cpu_mem_load(&cpu, &mem);
    return cpu;
}

static int mem_for(unsigned long total, unsigned long freeram)
{
    int cpu = -1, mem = -1;
    fake_si.totalram = total;
    fake_si.freeram = freeram;
    cpu_mem_load(&cpu, &mem);
    return mem;
}

void test_cpu_load_is_load_divided_by_cpu_count(void)
{
    TEST_ASSERT_EQUAL_INT(25, cpu_for(1.0, 4));
    TEST_ASSERT_EQUAL_INT(50, cpu_for(0.5, 1));
    TEST_ASSERT_EQUAL_INT(0,  cpu_for(0.0, 4));
}

void test_cpu_load_is_rounded(void)
{
    TEST_ASSERT_EQUAL_INT(33, cpu_for(1.0, 3));
    TEST_ASSERT_EQUAL_INT(67, cpu_for(2.0, 3));
}

void test_cpu_load_is_clamped_to_100(void)
{
    TEST_ASSERT_EQUAL_INT(100, cpu_for(8.0, 4));
    TEST_ASSERT_EQUAL_INT(100, cpu_for(4.0, 4));
}

void test_cpu_count_below_one_is_treated_as_one(void)
{
    TEST_ASSERT_EQUAL_INT(50, cpu_for(0.5, 0));
    TEST_ASSERT_EQUAL_INT(50, cpu_for(0.5, -1));
}

void test_mem_usage_percent(void)
{
    TEST_ASSERT_EQUAL_INT(75,  mem_for(1000, 250));
    TEST_ASSERT_EQUAL_INT(0,   mem_for(1000, 1000));
    TEST_ASSERT_EQUAL_INT(100, mem_for(1000, 0));
    TEST_ASSERT_EQUAL_INT(67,  mem_for(3, 1));
}

void test_sysinfo_failure_leaves_outputs_untouched(void)
{
    int cpu = -1, mem = -1;
    fake_sysinfo_ret = -1;
    cpu_mem_load(&cpu, &mem);
    TEST_ASSERT_EQUAL_INT(-1, cpu);
    TEST_ASSERT_EQUAL_INT(-1, mem);
}

void test_run_command_strips_trailing_newline(void)
{
    char out[64] = {0};
    set_output("hello\n");
    run_command("echo hello", out, sizeof(out));

    TEST_ASSERT_EQUAL_STRING("hello", out);
    TEST_ASSERT_EQUAL_STRING("echo hello", last_command);
    TEST_ASSERT_EQUAL_INT(1, pclose_count);
}

void test_run_command_popen_failure_leaves_output_untouched(void)
{
    char out[64] = "unchanged";
    fake_popen_fails = 1;
    run_command("anything", out, sizeof(out));

    TEST_ASSERT_EQUAL_STRING("unchanged", out);
    TEST_ASSERT_EQUAL_INT(0, pclose_count);
}

void test_gps_sync_true_when_enabled(void)
{
    set_output("ENABLED\n");
    TEST_ASSERT_TRUE(gps_sync());
}

void test_gps_sync_false_for_other_states(void)
{
    set_output("DISABLED\n");
    TEST_ASSERT_FALSE(gps_sync());
    set_output("NOT_PRESENT\n");
    TEST_ASSERT_FALSE(gps_sync());
}

void test_mesh_state_true_when_active(void)
{
    set_output("active\n");
    TEST_ASSERT_TRUE(mesh_state());
}

void test_mesh_state_false_when_not_active(void)
{
    set_output("inactive\n");
    TEST_ASSERT_FALSE(mesh_state());
    set_output("failed\n");
    TEST_ASSERT_FALSE(mesh_state());
}

void test_node_count_parses_integer(void)
{
    set_output("7\n");
    TEST_ASSERT_EQUAL_INT(7, node_count());

    set_output("0\n");
    TEST_ASSERT_EQUAL_INT(0, node_count());
}

void test_node_count_is_zero_for_non_numeric_output(void)
{
    set_output("error\n");
    TEST_ASSERT_EQUAL_INT(0, node_count());
}

#define RUN_MEAS_TEST(t) do { reset_fakes(); RUN_TEST(t); } while (0)

void run_measurement_tests(void)
{
    RUN_MEAS_TEST(test_cpu_load_is_load_divided_by_cpu_count);
    RUN_MEAS_TEST(test_cpu_load_is_rounded);
    RUN_MEAS_TEST(test_cpu_load_is_clamped_to_100);
    RUN_MEAS_TEST(test_cpu_count_below_one_is_treated_as_one);
    RUN_MEAS_TEST(test_mem_usage_percent);
    RUN_MEAS_TEST(test_sysinfo_failure_leaves_outputs_untouched);
    RUN_MEAS_TEST(test_run_command_strips_trailing_newline);
    RUN_MEAS_TEST(test_run_command_popen_failure_leaves_output_untouched);
    RUN_MEAS_TEST(test_gps_sync_true_when_enabled);
    RUN_MEAS_TEST(test_gps_sync_false_for_other_states);
    RUN_MEAS_TEST(test_mesh_state_true_when_active);
    RUN_MEAS_TEST(test_mesh_state_false_when_not_active);
    RUN_MEAS_TEST(test_node_count_parses_integer);
    RUN_MEAS_TEST(test_node_count_is_zero_for_non_numeric_output);
}
