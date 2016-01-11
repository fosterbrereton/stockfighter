/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef curl_hpp__
#define curl_hpp__

/******************************************************************************/

// libcurl
#include <curl/curl.h>

// stdc++
#include <mutex>

// application
#include "error.hpp"
#include "json_fwd.hpp"

/******************************************************************************/

struct curl_free_t {
    void operator()(void* x) {
        curl_free(x);
    }
};

using curl_string_t = std::unique_ptr<char, curl_free_t>;

/******************************************************************************/

struct curl_t {
private:
    static size_t read_callback(char* ptr, size_t size, size_t n, void* userdata) {
        return reinterpret_cast<curl_t*>(userdata)->read(ptr, size, n);
    }

    static size_t write_callback(char* ptr, size_t size, size_t n, void* userdata) {
        return reinterpret_cast<curl_t*>(userdata)->write(ptr, size, n);
    }

    static size_t header_callback(char* ptr, size_t size, size_t n, void* userdata) {
        return reinterpret_cast<curl_t*>(userdata)->header(ptr, size, n);
    }

public:
    curl_t() :
        curl_m(curl_easy_init()) {
        static std::once_flag global_state_flag_s;
        std::call_once(global_state_flag_s, [](){
           struct state_t {
               state_t() {
                   curl_global_init(CURL_GLOBAL_ALL);
               }
               ~state_t() {
                   curl_global_cleanup();
               }
           };

           static state_t state_s;
        });

        setopt(CURLOPT_READFUNCTION, &curl_t::read_callback);
        setopt(CURLOPT_READDATA, this);

        setopt(CURLOPT_WRITEFUNCTION, &curl_t::write_callback);
        setopt(CURLOPT_WRITEDATA, this);

        setopt(CURLOPT_HEADERFUNCTION, &curl_t::header_callback);
        setopt(CURLOPT_HEADERDATA, this);
    }

    ~curl_t() {
        curl_easy_cleanup(curl_m);

        if (headers_m) {
            curl_slist_free_all(headers_m);
        }
    }

    void set_url(const std::string& str) {
        setopt(CURLOPT_URL, str.c_str());
    }

    void set_post() {
        setopt(CURLOPT_POST, 1);
    }

    void set_user(const std::string& username) {
        setopt(CURLOPT_USERNAME, username.c_str());
    }

    void set_password(const std::string& password) {
        setopt(CURLOPT_PASSWORD, password.c_str());
    }

    void set_follow_location() {
        setopt(CURLOPT_FOLLOWLOCATION, 1);
    }

    void set_upload(std::string payload) {
        setopt(CURLOPT_UPLOAD, 1);

        payload_data_m = std::move(payload);
    }

    void set_post_data(const std::string& post_data) {
        post_data_m = post_data;

        setopt(CURLOPT_POSTFIELDS, post_data_m.c_str());
    }

    void set_header(const std::string& header) {
        headers_m = curl_slist_append(headers_m, header.c_str());
    }

    const std::string& perform() {
        if (headers_m) {
            setopt(CURLOPT_HTTPHEADER, headers_m);
        }

        curl_assert(curl_easy_perform(curl_m));

        long http_code(0);
        curl_easy_getinfo(curl_m, CURLINFO_RESPONSE_CODE, &http_code);
        response_code_m = http_code;

        return result();
    }

    std::size_t response_code() const {
        return response_code_m;
    }

    const std::string& headers() const {
        return header_data_m;
    }

    const std::string& result() const {
        return result_data_m;
    }

    std::string url_escape(const std::string& src) const {
        curl_string_t curl_out{curl_easy_escape(curl_m, src.c_str(), src.size())};

        return std::string(curl_out.get());
    }

    std::string url_unescape(const std::string& src) const {
        int           length(0);
        curl_string_t curl_out{curl_easy_unescape(curl_m, src.c_str(), src.size(), &length)};

        return std::string(curl_out.get(), length);
    }

    // Note: Maybe make this movable?
    curl_t(const curl_t&) = delete;
    curl_t(curl_t&&) = delete;
    curl_t& operator=(curl_t&&) = delete;
    curl_t& operator=(const curl_t&) = delete;

private:
    void curl_assert(CURLcode code) {
        if (code == CURLE_OK)
            return;

        throw_error(std::string("CURL : ") + curl_easy_strerror(code));
    }

    template <typename T, typename U>
    void setopt(T&& arg1, U&& arg2) {
        curl_assert(curl_easy_setopt(curl_m,
                                     std::forward<T>(arg1),
                                     std::forward<U>(arg2)));
    }

    size_t read(char* ptr, size_t size, size_t n) {
        if (payload_offset_m == payload_data_m.size())
            return 0;

        size_t count = size * n;
        size_t copycount = std::min(payload_offset_m + count, payload_data_m.size());

        std::memcpy(ptr, &payload_data_m[payload_offset_m], copycount);

        payload_offset_m += copycount;

        return copycount;
    }

    size_t write(char* ptr, size_t size, size_t n) {
        size_t count = size * n;

        result_data_m += std::string(ptr, ptr + count);

        return count;
    }

    size_t header(char* ptr, size_t size, size_t n) {
        size_t count = size * n;

        header_data_m += std::string(ptr, ptr + count);

        return count;
    }

    CURL*       curl_m{nullptr};
    curl_slist* headers_m{nullptr};
    std::string result_data_m;
    std::string header_data_m;
    std::string payload_data_m;
    std::size_t payload_offset_m{0};
    std::string post_data_m;
    std::size_t response_code_m{0};
};

/******************************************************************************/

std::string construct_full_url(const curl_t& curl,
                               std::string   url,
                               const json_t& parameters);

/******************************************************************************/

#endif // curl_hpp__

/******************************************************************************/
