#ifdef _WIN32
#include <Windows.h>
#include <fstream>
#include <filesystem>
#else
#include <curl/curl.h>
#endif
#include <string>
#include <vector>
#include <iostream>
#include <functional>

typedef std::function<void(double, double)> ProgressCallback;

#ifdef _WIN32
class Downloader : public IBindStatusCallback {
#else
class Downloader {
#endif
private:
    ProgressCallback progress_callback = nullptr;
    std::string _url;
#ifdef _WIN32
    STDMETHOD (OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR wszStatusText) {
        if (progress_callback != nullptr) progress_callback(ulProgress, ulProgressMax);
        return S_OK;
    }
    STDMETHOD (OnStartBinding)(DWORD dwReserved, IBinding __RPC_FAR* pib) { return E_NOTIMPL; }
    STDMETHOD (GetPriority)(LONG __RPC_FAR* pnPriority) { return E_NOTIMPL; }
    STDMETHOD (OnLowResource)(DWORD reserved) { return E_NOTIMPL; }
    STDMETHOD (OnStopBinding)(HRESULT hresult, LPCWSTR szError) { return E_NOTIMPL; }
    STDMETHOD (GetBindInfo)(DWORD __RPC_FAR* grfBINDF, BINDINFO __RPC_FAR* pbindinfo) { return E_NOTIMPL; }
    STDMETHOD (OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC __RPC_FAR* pformatetc, STGMEDIUM __RPC_FAR* pstgmed) { return E_NOTIMPL; }
    STDMETHOD (OnObjectAvailable)(REFIID riid, IUnknown __RPC_FAR* punk) { return E_NOTIMPL; }
    STDMETHOD_(ULONG,AddRef)() { return 0; }
    STDMETHOD_(ULONG,Release)() { return 0; }
    STDMETHOD (QueryInterface)(REFIID riid, void __RPC_FAR* __RPC_FAR* ppvObject) { return E_NOTIMPL; }
#else
    static size_t libcurl_write(void* contents, size_t size, size_t nmemb, std::vector<char>* buffer) {
        size_t totalSize = size * nmemb;
        buffer->insert(buffer->end(), (char*)contents, (char*)contents + totalSize);
        return totalSize;
    }
    static int libcurl_progress(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        Downloader* downloader = (Downloader*)clientp;
        if (downloader->progress_callback != nullptr) downloader->progress_callback(dlnow, dltotal);
        return 0;
    }
#endif
public:
    int status = -1;
    std::vector<char> data = {};
    Downloader(std::string url) {
        _url = url;
    }
    void progress(ProgressCallback callback) {
        progress_callback = callback;
    }
    void download() {
        std::cout << _url << "..." << std::flush;
        data.clear();
        std::string url = "";
        for (int i = 0; i < _url.length(); i++) {
            if (_url[i] == ' ') url += "%20";
            else url += _url[i];
        }
#ifdef _WIN32
        std::string destfile = std::string(std::getenv("TEMP")) + "/saturn-download-dest.dat";
        if (URLDownloadToFile(NULL, url.c_str(), destfile.c_str(), 0, this) == S_OK) {
            int filesize = std::filesystem::file_size(destfile);
            std::ifstream stream = std::ifstream(destfile, std::ios::binary);
            char* buf = (char*)malloc(filesize);
            stream.read(buf, filesize);
            stream.close();
            for (int i = 0; i < filesize; i++) {
                data.push_back(buf[i]);
            }
            status = 200;
        }
        else status = 0;
#else
        CURL* curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &libcurl_progress);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &libcurl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        struct curl_slist* header = nullptr;
        header = curl_slist_append(header, "Accept: */*");
        header = curl_slist_append(header, "User-Agent: saturn-studio-downloader");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        curl_easy_cleanup(curl);
        curl_slist_free_all(header);
#endif
        std::cout << status << std::endl;
    }
};
