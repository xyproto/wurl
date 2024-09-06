#include <curl/curl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

// Function to write data to file
static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    return fwrite(ptr, size, nmemb, stream);
}

// Function to display usage
void print_usage(const char* prog_name)
{
    fprintf(stderr, "Usage: %s [OPTION]... [URL]...\n", prog_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -O, --output-document=FILE  write documents to FILE.\n");
    fprintf(stderr, "  -c, --continue              resume getting a partially-downloaded file.\n");
    fprintf(stderr, "  -L, --location              follow redirects.\n");
    fprintf(stderr, "  -v, --verbose               turn on verbose output.\n");
    fprintf(stderr, "  -q, --quiet                 turn off all output.\n");
    fprintf(stderr, "  -d, --debug                 turn on debug output.\n");
    fprintf(stderr, "  --http-user=USER            set the HTTP username.\n");
    fprintf(stderr, "  --http-password=PASSWORD    set the HTTP password.\n");
    fprintf(stderr, "  --no-check-certificate      don't validate the SSL certificate.\n");
    fprintf(stderr, "  --proxy=URL                 use the specified proxy.\n");
    fprintf(stderr, "  --limit-rate=RATE           limit download speed to RATE (e.g., 200K, 1M).\n");
    fprintf(stderr, "  --retry=NUMBER              set number of retries to NUMBER (0 for infinite).\n");
    fprintf(stderr, "  -h, --help                  display this help and exit.\n");
}

// Function to parse rate limit (e.g., 200K, 1M)
curl_off_t parse_rate_limit(const char* rate)
{
    char* end;
    curl_off_t value = strtoll(rate, &end, 10);

    if (toupper(*end) == 'K') {
        value *= 1024;
    } else if (toupper(*end) == 'M') {
        value *= 1024 * 1024;
    }

    return value;
}

// Function to enable verbose, quiet, or debug modes
void set_output_mode(CURL* curl, int verbose, int quiet, int debug)
{
    if (quiet) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    } else if (debug) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    } else if (verbose) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }
}

int main(int argc, char* argv[])
{
    CURL* curl;
    FILE* fp;
    CURLcode res = CURLE_OK; // Initialize res to avoid warnings
    int opt;
    int option_index = 0;
    char* url = NULL;
    char* output_filename = NULL;
    char* http_user = NULL;
    char* http_password = NULL;
    char* proxy = NULL;
    char* limit_rate = NULL;
    int follow_location = 0;
    int resume_download = 0;
    int verbose = 0;
    int quiet = 0;
    int debug = 0;
    int no_check_certificate = 0;
    int retry_number = 20; // Default number of retries

    static struct option long_options[] = {
        { "output-document", required_argument, 0, 'O' },
        { "continue", no_argument, 0, 'c' },
        { "location", no_argument, 0, 'L' },
        { "verbose", no_argument, 0, 'v' },
        { "quiet", no_argument, 0, 'q' },
        { "debug", no_argument, 0, 'd' },
        { "http-user", required_argument, 0, 0 },
        { "http-password", required_argument, 0, 0 },
        { "no-check-certificate", no_argument, 0, 0 },
        { "proxy", required_argument, 0, 0 },
        { "limit-rate", required_argument, 0, 0 },
        { "retry", required_argument, 0, 0 },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    while ((opt = getopt_long(argc, argv, "O:cLvdqh", long_options, &option_index)) != -1) {
        switch (opt) {
        case 'O':
            output_filename = optarg;
            break;
        case 'c':
            resume_download = 1;
            break;
        case 'L':
            follow_location = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'q':
            quiet = 1;
            break;
        case 'd':
            debug = 1;
            break;
        case 0:
            if (strcmp(long_options[option_index].name, "http-user") == 0) {
                http_user = optarg;
            } else if (strcmp(long_options[option_index].name, "http-password") == 0) {
                http_password = optarg;
            } else if (strcmp(long_options[option_index].name, "no-check-certificate") == 0) {
                no_check_certificate = 1;
            } else if (strcmp(long_options[option_index].name, "proxy") == 0) {
                proxy = optarg;
            } else if (strcmp(long_options[option_index].name, "limit-rate") == 0) {
                limit_rate = optarg;
            } else if (strcmp(long_options[option_index].name, "retry") == 0) {
                retry_number = atoi(optarg);
            }
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            print_usage(argv[0]);
            return 1;
        }
    }

    if (optind < argc) {
        url = argv[optind];
    } else {
        fprintf(stderr, "URL is required.\n");
        print_usage(argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        if (!output_filename) {
            // Generate output filename from URL
            const char* last_slash = strrchr(url, '/');
            output_filename = last_slash ? strdup(last_slash + 1) : strdup("index.html");
        }

        fp = fopen(output_filename, resume_download ? "ab" : "wb");
        if (!fp) {
            perror("fopen");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return 1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Follow redirects if -L is used
        if (follow_location) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        }

        // Resume download if -c is used
        if (resume_download && output_filename) {
            fseek(fp, 0L, SEEK_END);
            long int resume_pos = ftell(fp);
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)resume_pos);
        }

        // Set HTTP authentication
        if (http_user && http_password) {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl, CURLOPT_USERNAME, http_user);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, http_password);
        }

        // Disable SSL certificate checking if --no-check-certificate is used
        if (no_check_certificate) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        // Set proxy if provided
        if (proxy) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
        }

        // Set download rate limit if --limit-rate is used
        if (limit_rate) {
            curl_off_t rate_limit = parse_rate_limit(limit_rate);
            curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, rate_limit);
        }

        // Set output mode
        set_output_mode(curl, verbose, quiet, debug);

        // Implement retry logic
        int retries = 0;
        do {
            res = curl_easy_perform(curl);
            if (res == CURLE_OK)
                break;

            retries++;
            fprintf(stderr, "Retrying %d/%d after failure: %s\n", retries, retry_number, curl_easy_strerror(res));
            sleep(1); // Simple retry delay
        } while (retries < retry_number);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        fclose(fp);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return res == CURLE_OK ? 0 : 1;
}
