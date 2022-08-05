#include "signer.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include "RestfulApiCommon.h"
#include "adapter.h"

using namespace std;
using namespace aliyun_log_sdk_v6;
using namespace aliyun_log_sdk_v6::auth;

namespace aliyun_log_sdk_v6 {
namespace auth {
extern const char* const EMPTY_STRING_SHA256 =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
} // end of namespace auth
} // end of namespace aliyun_log_sdk_v6

// ====================== sign v1 ===========================

void SignerV1::Sign(const string& httpMethod,
                    const string& resourceUri,
                    map<string, string>& httpHeaders,
                    const map<string, string>& urlParams,
                    const string& content) {
    string contentMd5, signature, osstream, contentType;
    string contentLength = std::to_string(content.size());
    // content md5
    if (!content.empty()) {
        contentMd5 = CodecTool::CalcMD5(content);
    }
    string dateTime = GetDateTimeString();
    // set http header
    if (httpHeaders.find(DATE) == httpHeaders.end())
        httpHeaders[DATE] = dateTime;
    else
        dateTime = httpHeaders[DATE];
    httpHeaders[CONTENT_LENGTH] = contentLength;

    auto iter = httpHeaders.find(CONTENT_TYPE);
    if (iter != httpHeaders.end()) {
        contentType = iter->second;
    }
    std::map<string, string> endingMap;
    osstream.append(httpMethod).append("\n");
    osstream.append(contentMd5).append("\n");
    osstream.append(contentType).append("\n");
    osstream.append(dateTime).append("\n");
    // header
    for (auto& iter : httpHeaders) {
        if (CodecTool::StartWith(iter.first, LOG_OLD_HEADER_PREFIX)) {
            std::string key = iter.first;
            endingMap.insert(std::make_pair(
                key.replace(0, std::strlen(LOG_OLD_HEADER_PREFIX),
                            LOG_HEADER_PREFIX),
                iter.second));
        } else if (CodecTool::StartWith(iter.first, LOG_HEADER_PREFIX) ||
                   CodecTool::StartWith(iter.first, ACS_HEADER_PREFIX)) {
            endingMap.insert(std::make_pair(iter.first, iter.second));
        }
    }
    for (auto& it : endingMap) {
        osstream.append(it.first).append(":").append(it.second);
        osstream.append("\n");
    }
    // uri and url params
    osstream.append(resourceUri);
    if (urlParams.size() > 0)
        osstream.append("?");
    for (auto iter = urlParams.begin(); iter != urlParams.end(); ++iter) {
        if (iter != urlParams.begin()) {
            osstream.append("&");
        }
        osstream.append(iter->first).append("=").append(iter->second);
    }
    // calc signature and write into header
    signature = CodecTool::Base64Enconde(
        CodecTool::CalcHMACSHA1(osstream, mCredential.accessKeySecret));
    httpHeaders[AUTHORIZATION] =
        LOG_HEADSIGNATURE_PREFIX + mCredential.accessKeyId + ':' + signature;
}

string SignerV1::GetDateTimeString() {
    return CodecTool::GetDateString(DATE_FORMAT_RFC822);
}

// ====================== sign v4 ===========================

void SignerV4::Sign(const string& httpMethod,
                    const string& resourceUri,
                    map<string, string>& httpHeaders,
                    const map<string, string>& urlParams,
                    const string& payload) {
    string date = GetDateString(), dateTime = GetDateTimeString();
    string contentLength = std::to_string(payload.size());

    // hexed sha256 of payload(http body)
    string sha256Payload = EMPTY_STRING_SHA256;
    if (!payload.empty()) {
        sha256Payload = CodecTool::ToHex(SHA256()(payload));
    }

    // set http header
    httpHeaders[X_LOG_CONTENT_SHA256] = sha256Payload;
    httpHeaders[X_LOG_DATE] = dateTime;
    httpHeaders[CONTENT_LENGTH] = contentLength;

    // canonicalHeaders & signedHeaderStr
    auto canonicalHeaders = BuildCanonicalHeaders(httpHeaders);
    string signedHeaderStr = BuildSignedHeaderStr(canonicalHeaders);
    // canonicalRequest
    string canonicalRequest =
        BuildCanonicalRequest(httpMethod, resourceUri, signedHeaderStr,
                              sha256Payload, urlParams, canonicalHeaders);
    string scope = BuildScope(date);

    // signKey + signMessage ==hmac-sha256===> signature
    string signMessage = BuildSignMessage(canonicalRequest, dateTime, scope);
    string signKey = BuildSignKey(mCredential.accessKeySecret, mRegion, date);
    string signature = CodecTool::ToHex(
        CodecTool::CalcHMACSHA256(signMessage, signKey));

    // signature => authorization header
    string authorization = BuildAuthorization(
        mCredential.accessKeyId, signedHeaderStr, signature, scope);

    // write to header
    httpHeaders[AUTHORIZATION] = authorization;

    // todo: comment this
    cout << "[sha256Payload]:" << endl << sha256Payload << endl;
    cout << "[dateTime]:" << endl << dateTime << endl;
    cout << "[date]:" << endl << date << endl;
    cout << "[scope]:" << endl << scope << endl;
    cout << "[signedHeaderStr]:" << endl << signedHeaderStr << endl;
    cout << "[canonicalRequest]:" << endl << canonicalRequest << endl;
    cout << "[signMessage]:" << endl << signMessage << endl;
    cout << "[signKey]:" << endl << CodecTool::ToHex(signKey) << endl;
    cout << "[signature]:" << endl << signature << endl;
    cout << "[authorization]:" << endl << authorization << endl;
}

map<string, string> SignerV4::BuildCanonicalHeaders(
    const map<string, string>& httpHeaders) {
    map<string, string> canonicalHeaders;
    for (auto& it : httpHeaders) {
        auto lowerKey = CodecTool::LowerCase(it.first);
        if (DEFAULT_SIGNED_HEADERS.count(lowerKey) ||
            lowerKey.find(LOG_HEADER_PREFIX) == 0 ||
            lowerKey.find(ACS_HEADER_PREFIX) == 0) {
            canonicalHeaders[lowerKey] = it.second;
        }
    }
    return canonicalHeaders;
}

string SignerV4::BuildSignedHeaderStr(
    const map<string, string>& canonicalHeaders) {
    stringstream ss;
    string separator = "";
    for (auto& it : canonicalHeaders) {
        ss << separator << it.first;
        separator = ";";
    }
    return ss.str();
}

string SignerV4::GetDateString() {
    return CodecTool::GetDateString(DATE_FORMAT_ISO8601);
}
string SignerV4::GetDateTimeString() {
    return CodecTool::GetDateString(DATE_FORMAT_ISO8601);
}

// canonical request + date + scope ==>> signMessage
string SignerV4::BuildCanonicalRequest(
    const string& method,
    const string& resourceUri,
    const string& signedHeaderStr,
    const string& sha256Payload,
    const map<string, string>& urlParams,
    const map<string, string>& canonicalHeaders) {
    stringstream ss;
    // http method + "\n"
    ss << method << "\n";

    // canonical resource uri (path) + "\n"
    ss << UrlEncode(resourceUri, true) << "\n";

    // canonical url params + "\n"
    map<string, string> canoUrlParams;  // trimed and url encoded
    for (auto& it : urlParams) {
        canoUrlParams[UrlEncode(CodecTool::Trim(it.first), false)] =
            UrlEncode(CodecTool::Trim(it.second), false);
    }
    string separator = "";
    stringstream us;  // cano url stringstream
    for (auto& it : canoUrlParams) {
        us << separator << it.first;
        if (!it.second.empty())
            us << "=" << it.second;
        separator = "&";
    }
    ss << us.str() << "\n";

    // canonical header + "\n"
    stringstream hs;  // cano header stream
    for (auto& it : canonicalHeaders) {
        hs << it.first << ":" << CodecTool::Trim(it.second) << "\n";
    }
    ss << hs.str() << "\n";

    // signed header + "\n"
    ss << signedHeaderStr << "\n";

    // sha256 payload, without "\n"
    ss << sha256Payload;

    return ss.str();
}

string SignerV4::BuildScope(const string& date) {
    return date + "/" + mRegion + "/" + SLS_PRODUCT_NAME + "/" + TERMINATOR;
}

string SignerV4::BuildSignMessage(const string& canonicalRequest,
                                  const string& dateTime,
                                  const string& scope) {
    return SLS4_HMAC_SHA256 + "\n" + dateTime + "\n" + scope + "\n" +
           CodecTool::ToHex(SHA256()(canonicalRequest));
}

string SignerV4::BuildSignKey(const string& accessKeySecret,
                              const string& region,
                              const string& date) {
    string signedDate =
        CodecTool::CalcHMACSHA256(date, SECRET_KEY_PREFIX + accessKeySecret);
    string signedRegion = CodecTool::CalcHMACSHA256(region, signedDate);
    string signedService =
        CodecTool::CalcHMACSHA256(SLS_PRODUCT_NAME, signedRegion);
    return CodecTool::CalcHMACSHA256(TERMINATOR, signedService);
}

// accessKeyId + signHeaderStr + signature + scope ===> final authorization
string SignerV4::BuildAuthorization(const string& accessKeyId,
                                    const string& signHeaderStr,
                                    const string& signature,
                                    const string& scope) {
    string credential = "Credential=" + accessKeyId + "/" + scope;
    string signedHeaders = "SignedHeaders=" + signHeaderStr;
    string sign = "Signature=" + signature;
    return SLS4_HMAC_SHA256 + " " + credential + "," + signedHeaders + "," +
           sign;
}

const string SignerV4::SECRET_KEY_PREFIX = "aliyun_v4";
const string SignerV4::SLS_PRODUCT_NAME = "sls";
const string SignerV4::TERMINATOR = "aliyun_v4_request";
const string SignerV4::SLS4_HMAC_SHA256 = "SLS4-HMAC-SHA256";
const unordered_set<string> SignerV4::DEFAULT_SIGNED_HEADERS = {
    CodecTool::LowerCase(CONTENT_TYPE), CodecTool::LowerCase(HOST)};
const unordered_map<string, string> SignerV4::CHARACTER_WITHOUT_SLASH = {
    {"+", "%20"},
    {"*", "%2A"},
    {"%7E", "~"}};
const unordered_map<string, string> SignerV4::CHARACTER_WITH_SLASH = {
    {"+", "%20"},
    {"*", "%2A"},
    {"%7E", "~"},
    {"%2F", "/"}};

string SignerV4::UrlEncode(const string& val, bool ignoreSlash) {
    if (val.empty())
        return "";
    string encoded = CodecTool::UrlEncode(val);

    // for each {oldStr, newStr} in map, do res = replaceAll(res, oldStr,
    // newStr)
    auto replaceEachInMap =
        [](const string& value,
                      const unordered_map<string, string>& forReplace) {
            string res = value;
            for (auto& it : forReplace) {
                res = CodecTool::ReplaceAll(res, it.first, it.second);
            }
            return res;
        };
    if (!ignoreSlash)
        return replaceEachInMap(val, CHARACTER_WITHOUT_SLASH);
    return replaceEachInMap(val, CHARACTER_WITH_SLASH);
}

// ================ Signer =================
const string Signer::DEFAULT_EMPTY_REGION = "";

void Signer::Sign(const string& httpMethod,
                  const string& resourceUri,
                  map<string, string>& httpHeaders,
                  const map<string, string>& urlParams,
                  const string& payload,
                  const string& accessKeySecret,
                  const string& accessKeyId,
                  const LOGSignType signType,
                  const string& region) {
    switch (signType) {
        case SIGN_V1: {
            auth::SignerV1 signer({accessKeyId, accessKeySecret});
            signer.Sign(httpMethod, resourceUri, httpHeaders, urlParams,
                        payload);
            break;
        }

        case SIGN_V4: {
            if (region.empty())
                throw LOGException(LOGE_SIGNV4_REGION_REQUIRED,
                                   "Signature Version 4 need param 'region'.");
            auth::SignerV4 signer({accessKeyId, accessKeySecret}, region);
            signer.Sign(httpMethod, resourceUri, httpHeaders, urlParams,
                        payload);
            break;
        }

        default:
            throw LOGException(LOGE_NOT_IMPLEMENTED,
                               "Signature Version does not support.");
    }
}