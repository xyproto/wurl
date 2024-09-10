#include <ctype.h>
#include <curl/curl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION "wurl 0.0.1"

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
    fprintf(stderr, "  --user-agent=STRING         set the User-Agent header.\n");
    fprintf(stderr, "  --referer=STRING            set the Referer header.\n");
    fprintf(stderr, "  --timeout=SECONDS           set the timeout for the entire transfer.\n");
    fprintf(stderr, "  --dns-timeout=SECONDS       set the DNS lookup timeout.\n");
    fprintf(stderr, "  --connect-timeout=SECONDS   set the connection timeout.\n");
    fprintf(stderr, "  --read-timeout=SECONDS      set the read timeout.\n");
    fprintf(stderr, "  --no-proxy                  don't use proxy, even if set in environment.\n");
    fprintf(stderr, "  --header=STRING             add custom header to the HTTP request.\n");
    fprintf(stderr, "  -4, --inet4-only            connect to IPv4 addresses only.\n");
    fprintf(stderr, "  -6, --inet6-only            connect to IPv6 addresses only.\n");
    fprintf(stderr, "  -h, --help                  display this help and exit.\n");
    fprintf(stderr, "  --version                   output version information and exit.\n");
}

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

int main(int argc, char* argv[])
{
    CURL* curl;
    FILE* fp;
    CURLcode res = CURLE_OK;
    int opt;
    int option_index = 0;
    char* url = NULL;
    const char* output_filename = NULL;
    char* http_user = NULL;
    char* http_password = NULL;
    char* proxy = NULL;
    char* limit_rate = NULL;
    char* user_agent = NULL;
    char* referer = NULL;
    int follow_location = 0;
    int resume_download = 0;
    int verbose = 0;
    int quiet = 0;
    int debug = 0;
    int no_check_certificate = 0;
    int retry_number = 20;
    long timeout = 0;
    long dns_timeout = 0;
    long connect_timeout = 0;
    long read_timeout = 0;
    int ipv4_only = 0;
    int ipv6_only = 0;
    struct curl_slist* headers = NULL;

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
        { "user-agent", required_argument, 0, 0 },
        { "referer", required_argument, 0, 0 },
        { "timeout", required_argument, 0, 0 },
        { "dns-timeout", required_argument, 0, 0 },
        { "connect-timeout", required_argument, 0, 0 },
        { "read-timeout", required_argument, 0, 0 },
        { "no-proxy", no_argument, 0, 0 },
        { "header", required_argument, 0, 0 },
        { "inet4-only", no_argument, 0, '4' },
        { "inet6-only", no_argument, 0, '6' },
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 0 },
        { 0, 0, 0, 0 }
    };

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    while ((opt = getopt_long(argc, argv, "O:cLvdqh46", long_options, &option_index)) != -1) {
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
        case '4':
            ipv4_only = 1;
            break;
        case '6':
            ipv6_only = 1;
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
            } else if (strcmp(long_options[option_index].name, "user-agent") == 0) {
                user_agent = optarg;
            } else if (strcmp(long_options[option_index].name, "referer") == 0) {
                referer = optarg;
            } else if (strcmp(long_options[option_index].name, "timeout") == 0) {
                timeout = atol(optarg);
            } else if (strcmp(long_options[option_index].name, "dns-timeout") == 0) {
                dns_timeout = atol(optarg);
            } else if (strcmp(long_options[option_index].name, "connect-timeout") == 0) {
                connect_timeout = atol(optarg);
            } else if (strcmp(long_options[option_index].name, "read-timeout") == 0) {
                read_timeout = atol(optarg);
            } else if (strcmp(long_options[option_index].name, "no-proxy") == 0) {
                curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");
            } else if (strcmp(long_options[option_index].name, "header") == 0) {
                headers = curl_slist_append(headers, optarg);
            } else if (strcmp(long_options[option_index].name, "version") == 0) {
                printf("%s\n", VERSION);
                return 0;
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

    if (curl) {
        if (!output_filename) {
            const char* last_slash = strrchr(url, '/');
            output_filename = last_slash ? last_slash + 1 : "index.html";
        }

        fp = fopen(output_filename, resume_download ? "ab" : "wb");
        if (!fp) {
            perror("fopen");
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return 1;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        if (follow_location) {
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        }

        if (resume_download && output_filename) {
            fseek(fp, 0L, SEEK_END);
            long int resume_pos = ftell(fp);
            curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)resume_pos);
        }

        if (http_user && http_password) {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
            curl_easy_setopt(curl, CURLOPT_USERNAME, http_user);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, http_password);
        }

        if (no_check_certificate) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        if (proxy) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
        }

        if (limit_rate) {
            curl_off_t rate_limit = parse_rate_limit(limit_rate);
            curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, rate_limit);
        }

        if (user_agent) {
            curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);
        }

        if (referer) {
            curl_easy_setopt(curl, CURLOPT_REFERER, referer);
        }

        if (timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        }

        if (dns_timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, dns_timeout);
        }

        if (connect_timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
        }

        if (read_timeout > 0) {
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, read_timeout);
        }

        if (ipv4_only) {
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        } else if (ipv6_only) {
            curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
        }

        if (headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }

        if (quiet) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        } else if (debug) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        } else if (verbose) {
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        int retries = 0;
        do {
            res = curl_easy_perform(curl);
            if (res == CURLE_OK)
                break;

            retries++;
            fprintf(stderr, "Retrying %d/%d after failure: %s\n", retries, retry_number, curl_easy_strerror(res));
            sleep(1);
        } while (retries < retry_number);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        fclose(fp);
        curl_easy_cleanup(curl);
        if (headers) {
            curl_slist_free_all(headers);
        }
    }

    curl_global_cleanup();
    return res == CURLE_OK ? 0 : 1;
}
