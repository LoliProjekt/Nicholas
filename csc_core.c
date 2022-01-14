#include <core.h>
#include <curl/curl.h>
#include <json-c/json_tokener.h>
#include <stdlib.h>
#include <string.h>

#define CSC_API_URL "https://capi-v2.sankakucomplex.com"

json_object *csc_request(long timeout, const char *api_data, ...) {
    va_list args;
    va_start(args, api_data);

    char api_data_args[1024];
    vsnprintf(api_data_args, sizeof(api_data_args), api_data, args);

    va_end(args);

    size_t length = bot_strlen(CSC_API_URL) + bot_strlen(api_data_args) + 2;
    char *csc_url = malloc(sizeof(char) * length);
    if(!csc_url)
        return 0;

    snprintf(csc_url, length, "%s/%s", CSC_API_URL, api_data_args);

    CURL *get_data = curl_easy_init();
    extern char csc_authorization_header[512];
    struct curl_slist *slist_auth = curl_slist_append(0, csc_authorization_header);
    struct bot_curl_string string = {0};

    curl_easy_setopt(get_data, CURLOPT_URL, csc_url);
    curl_easy_setopt(get_data, CURLOPT_HTTPHEADER, slist_auth);
    curl_easy_setopt(get_data, CURLOPT_USERAGENT, "Nicholas");
    curl_easy_setopt(get_data, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(get_data, CURLOPT_WRITEFUNCTION, bot_curl_writefunction);
    curl_easy_setopt(get_data, CURLOPT_WRITEDATA, &string);
    if(timeout) {
        curl_easy_setopt(get_data, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(get_data, CURLOPT_CONNECTTIMEOUT, timeout);
    }
    curl_easy_perform(get_data);

    curl_slist_free_all(slist_auth);
    curl_easy_cleanup(get_data);
    free(csc_url);

    if(!string.string)
        return 0;

    json_object *data = json_tokener_parse(string.string);
    free(string.string);

    return data;
}

void bot_csc_post(char *csc_info, size_t length, json_object *csc_data, int csc_id) {
    const char *csc_rating = json_object_get_string(json_object_object_get(csc_data, "rating"));
    char *csc_author = bot_strenc(json_object_get_string(json_object_object_get(json_object_object_get(csc_data, "author"), "name")), 256);
    float csc_size = json_object_get_int(json_object_object_get(csc_data, "file_size"));
    const char *csc_filetype = json_object_get_string(json_object_object_get(csc_data, "file_type"));
    time_t rawtime = json_object_get_int(json_object_object_get(json_object_object_get(csc_data, "created_at"), "s"));
    struct tm *date = localtime(&rawtime);
    const char *csc_parent_id = json_object_get_string(json_object_object_get(csc_data, "parent_id"));
    float csc_vote_count = json_object_get_int(json_object_object_get(csc_data, "vote_count"));
    float csc_vote_average = json_object_get_int(json_object_object_get(csc_data, "total_score")) / csc_vote_count;
    const char *csc_source = json_object_get_string(json_object_object_get(csc_data, "source"));

    char *csc_rating_s;
    
    if(!bot_strcmp(csc_rating, "s"))
        csc_rating_s = "safe";
    else if(!bot_strcmp(csc_rating, "q"))
        csc_rating_s = "questionable";
    else if(!bot_strcmp(csc_rating, "e"))
        csc_rating_s = "explicit";
    else
        csc_rating_s = "unknown";

    char csc_size_s[16] = "";

    if(csc_size > 1073741824)
        snprintf(csc_size_s, sizeof(csc_size_s), "(%.2f GiB)", csc_size / 1073741824);
    else if(csc_size > 1048576)
        snprintf(csc_size_s, sizeof(csc_size_s), "(%.2f MiB)", csc_size / 1048576);
    else if(csc_size > 1024)
        snprintf(csc_size_s, sizeof(csc_size_s), "(%.2f KiB)", csc_size / 1024);

    char *csc_format, csc_sample_url[512];

    if(csc_filetype) {
        if(!bot_strcmp(csc_filetype, "image/jpeg")) {
            csc_format = "JPEG";
            bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "sample_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "image/png")) {
            csc_format = "PNG";
            bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "sample_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "image/gif")) {
            csc_format = "GIF";
            if(csc_size <= 20971520)
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "file_url")), sizeof(csc_sample_url));
            else
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "preview_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "video/mp4")) {
            csc_format = "MP4";
            if(csc_size <= 20971520)
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "file_url")), sizeof(csc_sample_url));
            else
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "preview_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "video/webm")) {
            csc_format = "WEBM";
            bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(csc_data, "preview_url")), sizeof(csc_sample_url));
        } else {
            csc_format = "unknown";
            bot_strncpy(csc_sample_url, "https://s.sankakucomplex.com/download-preview.png", sizeof(csc_sample_url));
        }
    } else {
        csc_format = "SWF";
        bot_strncpy(csc_sample_url, "https://s.sankakucomplex.com/download-preview.png", sizeof(csc_sample_url));
    }

    char *csc_has_children_s;

    if(json_object_get_boolean(json_object_object_get(csc_data, "has_children")))
        csc_has_children_s = "yes";
    else
        csc_has_children_s = "no";

    char csc_parent_id_s[32];

    if(csc_parent_id)
        snprintf(csc_parent_id_s, sizeof(csc_parent_id_s), "<code>%s</code>", csc_parent_id);
    else
        bot_strncpy(csc_parent_id_s, "none", sizeof(csc_parent_id_s));

    if(csc_vote_average != (float)csc_vote_average)
        csc_vote_average = 0;

    char csc_source_s[1024];

    if(csc_source) {
        if(csc_source[0])
            snprintf(csc_source_s, sizeof(csc_source_s), "%s", csc_source);
        else
            bot_strncpy(csc_source_s, "none", sizeof(csc_source_s));
    } else {
        bot_strncpy(csc_source_s, "none", sizeof(csc_source_s));
    }

    char csc_date[16];
    strftime(csc_date, sizeof(csc_date), "%d.%m.%Y", date);

    char  csc_time[16];
    strftime(csc_time, sizeof(csc_time), "%T %Z", date);

    snprintf(csc_info, length, "<a href=\"%s\">&#8203;</a><b>ID:</b> <code>%d</code>\n<b>Rating:</b> %s\n<b>Status:</b> %s\n<b>Author:</b> %s\n<b>Sample resolution:</b> %sx%s\n<b>Resolution:</b> %sx%s\n<b>Size:</b> <code>%.0f</code> bytes %s\n<b>Type:</b> %s\n<b>Date:</b> <code>%s</code> %s\n<b>Has children:</b> %s\n<b>Parent ID:</b> %s\n<b>MD5:</b> <code>%s</code>\n<b>Fav count:</b> %d\n<b>Vote count:</b> %.0f\n<b>Vote average:</b> %.2f\n<b>Source:</b> %s", csc_sample_url, csc_id, csc_rating_s, json_object_get_string(json_object_object_get(csc_data, "status")), csc_author, json_object_get_string(json_object_object_get(csc_data, "sample_width")), json_object_get_string(json_object_object_get(csc_data, "sample_height")), json_object_get_string(json_object_object_get(csc_data, "width")), json_object_get_string(json_object_object_get(csc_data, "height")), csc_size, csc_size_s, csc_format, csc_date, csc_time, csc_has_children_s, csc_parent_id_s, json_object_get_string(json_object_object_get(csc_data, "md5")), json_object_get_int(json_object_object_get(csc_data, "fav_count")), csc_vote_count, csc_vote_average, csc_source_s);
    bot_free(1, csc_author);
}

void bot_csc_pool(char *csc_info, size_t length, json_object *csc_data, int csc_id) {
    const char *csc_description = json_object_get_string(json_object_object_get(csc_data, "description"));
    char *csc_author = bot_strenc(json_object_get_string(json_object_object_get(json_object_object_get(csc_data, "author"), "name")), 256);
    const char *csc_rating = json_object_get_string(json_object_object_get(csc_data, "rating"));
    float csc_vote_count = json_object_get_int(json_object_object_get(csc_data, "vote_count"));
    float csc_vote_average = json_object_get_int(json_object_object_get(csc_data, "total_score")) / csc_vote_count;
    json_object *cover_post = json_object_object_get(csc_data, "cover_post");
    float csc_size = json_object_get_int(json_object_object_get(cover_post, "file_size"));
    const char *csc_filetype = json_object_get_string(json_object_object_get(cover_post, "file_type"));
    char *csc_name = bot_strenc(json_object_get_string(json_object_object_get(csc_data, "name")), 1024);

    char csc_description_s[11264];

    if(csc_description[0]) {
        char *csc_description_encoded = bot_strenc(csc_description, 2048);
        snprintf(csc_description_s, sizeof(csc_description_s), "<code>%s</code>", csc_description_encoded);
        bot_free(1, csc_description_encoded);
    } else {
        bot_strncpy(csc_description_s, "none", sizeof(csc_description));
    }

    char *csc_rating_s;

    if(!bot_strcmp(csc_rating, "s"))
        csc_rating_s = "safe";
    else if(!bot_strcmp(csc_rating, "q"))
        csc_rating_s = "questionable";
    else if(!bot_strcmp(csc_rating, "e"))
        csc_rating_s = "explicit";
    else
        csc_rating_s = "unknown";

    char csc_sample_url[512];

    if(csc_filetype) {
        if(!bot_strcmp(csc_filetype, "image/jpeg")) {
            bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "sample_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "image/png")) {
            bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "sample_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "image/gif")) {
            if(csc_size <= 20971520)
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "file_url")), sizeof(csc_sample_url));
            else
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "preview_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "video/mp4")) {
            if(csc_size <= 20971520)
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "file_url")), sizeof(csc_sample_url));
            else
                bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "preview_url")), sizeof(csc_sample_url));
        } else if(!bot_strcmp(csc_filetype, "video/webm")) {
            bot_strncpy(csc_sample_url, json_object_get_string(json_object_object_get(cover_post, "preview_url")), sizeof(csc_sample_url));
        } else {
            bot_strncpy(csc_sample_url, "https://s.sankakucomplex.com/download-preview.png", sizeof(csc_sample_url));
        }
    } else {
        bot_strncpy(csc_sample_url, "https://s.sankakucomplex.com/download-preview.png", sizeof(csc_sample_url));
    }

    if(csc_vote_average != (float)csc_vote_average)
        csc_vote_average = 0;

    snprintf(csc_info, length, "<a href=\"%s\">&#8203;</a><b>ID:</b> <code>%d</code>\n<b>Description:</b> %s\n<b>Date:</b> %s\n<b>Author:</b> %s\n<b>Pages:</b> %d\n<b>Rating:</b> %s\n<b>Vote count:</b> %.0f\n<b>Vote average:</b> %.2f\n<b>Cover post ID:</b> <code>%d</code>\n<b>Name:</b> <code>%s</code>", csc_sample_url, csc_id, csc_description_s, json_object_get_string(json_object_object_get(csc_data, "created_at")), csc_author, json_object_get_int(json_object_object_get(csc_data, "visible_post_count")), csc_rating_s, csc_vote_count, csc_vote_average, json_object_get_int(json_object_object_get(cover_post, "id")), csc_name);
    bot_free(2, csc_author, csc_name);
}

void bot_csc_tag(char *csc_tag, size_t length, json_object *csc_data, int csc_id) {
    const char *csc_name_en = json_object_get_string(json_object_object_get(csc_data, "name_en"));
    const char *csc_name_ja = json_object_get_string(json_object_object_get(csc_data, "name_ja"));
    char *csc_name = bot_strenc(json_object_get_string(json_object_object_get(csc_data, "name")), 1024);
    const char *csc_rating = json_object_get_string(json_object_object_get(csc_data, "rating"));

    char csc_name_en_s[6144];
    
    if(csc_name_en) {
        char *csc_name_en_encoded = bot_strenc(csc_name_en, 1024);
        snprintf(csc_name_en_s, sizeof(csc_name_en_s), "<code>%s</code>", csc_name_en_encoded);
        bot_free(1, csc_name_en_encoded);
    } else {
        bot_strncpy(csc_name_en_s, "none", sizeof(csc_name_en_s));
    }

    char csc_name_ja_s[6144];
    
    if(csc_name_ja) {
        char *csc_name_ja_encoded = bot_strenc(csc_name_ja, 1024);
        snprintf(csc_name_ja_s, sizeof(csc_name_ja_s), "<code>%s</code>", csc_name_ja_encoded);
        bot_free(1, csc_name_ja_encoded);
    } else {
        bot_strncpy(csc_name_ja_s, "none", sizeof(csc_name_ja_s));
    }

    char *csc_rating_s;

    if(csc_rating) {
        if(!bot_strcmp(csc_rating, "s"))
            csc_rating_s = "safe";
        else if(!bot_strcmp(csc_rating, "q"))
            csc_rating_s = "questionable";
        else if(!bot_strcmp(csc_rating, "e"))
            csc_rating_s = "explicit";
        else
            csc_rating_s = "unknown";
    } else {
        csc_rating_s = "none";
    }

    char *csc_type_s;

    switch(json_object_get_int(json_object_object_get(csc_data, "type"))) {
        case 0:
            csc_type_s = "general";
            break;
        case 1:
            csc_type_s = "artist";
            break;
        case 2:
            csc_type_s = "studio";
            break;
        case 3:
            csc_type_s = "copyright";
            break;
        case 4:
            csc_type_s = "character";
            break;
        case 5:
            csc_type_s = "genre";
            break;
        case 8:
            csc_type_s = "medium";
            break;
        case 9:
            csc_type_s = "meta";
            break;
        default:
            csc_type_s = "unknown";
            break;
    }

    snprintf(csc_tag, length, "<b>ID:</b> <code>%d</code>\n<b>Eng name:</b> %s\n<b>Jap name:</b> %s\n<b>Type:</b> %s\n<b>Post count:</b> %d\n<b>Book count:</b> %d\n<b>Rating:</b> %s\n<b>Name:</b> <code>%s</code>", csc_id, csc_name_en_s, csc_name_ja_s, csc_type_s, json_object_get_int(json_object_object_get(csc_data, "post_count")), json_object_get_int(json_object_object_get(csc_data, "pool_count")), csc_rating_s, csc_name);
    bot_free(1, csc_name);
}
