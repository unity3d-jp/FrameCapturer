#include "pch.h"
#define CURL_STATICLIB
#include <curl/curl.h>
#include "Misc.h"


struct HTTPContext
{
    const HTTPCallback& callback;
};

static int HTTPCalback_Impl(char* data, size_t size, size_t nmemb, HTTPContext *ctx)
{
    size_t len = size * nmemb;
    if (ctx->callback) {
        // abort if callback returned false
        if (!ctx->callback(data, len)) {
            return 0;
        }
    }
    return (int)len;
}

static bool HTTPGet_Impl(const std::string &url, HTTPContext &ctx)
{
    CURL *curl = curl_easy_init();
    if (curl == nullptr) { return false; }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &HTTPCalback_Impl);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    bool ret = false;
    auto curl_code = curl_easy_perform(curl);
    if (curl_code == CURLE_OK) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        ret = http_code == 200;
    }
    else {
        // curl_easy_strerror(curl_code);
    }
    curl_easy_cleanup(curl);
    return ret;
}

bool HTTPGet(const std::string &url, std::string &response)
{
    HTTPCallback callback = [&](const char* data, size_t size) {
        response.append(data, size);
        return true;
    };
    HTTPContext ctx = { callback };
    return HTTPGet_Impl(url, ctx);
}

bool HTTPGet(const std::string &url, const HTTPCallback& callback)
{
    HTTPContext ctx = { callback };
    return HTTPGet_Impl(url, ctx);
}
