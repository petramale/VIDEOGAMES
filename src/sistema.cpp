#include "sistema.h"
#include <cstdlib>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

// Gli header di MinGW inclusi in Dev-C++ (TDM-GCC 9.2) sono piu' vecchi di Windows 8.1
// e non conoscono questa costante: la definiamo noi con il valore ufficiale.
#ifndef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#endif

std::string PercorsoEseguibile() {
    char buf[MAX_PATH * 2];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    return std::string(buf, n);
}

namespace {
struct StatoHttp {
    volatile LONG stato = 0;
    std::wstring host, percorso, intestazioni;
    std::string corpo, risposta;
} http;

std::wstring Largo(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

DWORD WINAPI ThreadHttp(LPVOID) {
    std::string risposta, errore;
    HINTERNET sessione = WinHttpOpen(L"StudioVideogiochi/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                     WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!sessione) // Windows piu' vecchi non conoscono AUTOMATIC_PROXY
        sessione = WinHttpOpen(L"StudioVideogiochi/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET conn = sessione ? WinHttpConnect(sessione, http.host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0) : nullptr;
    HINTERNET req = conn ? WinHttpOpenRequest(conn, L"POST", http.percorso.c_str(), nullptr, WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    bool ok = req != nullptr;
    if (ok) {
        WinHttpSetTimeouts(req, 10000, 10000, 30000, 120000);
        ok = WinHttpSendRequest(req, http.intestazioni.c_str(), (DWORD)-1L, (LPVOID)http.corpo.data(),
                                (DWORD)http.corpo.size(), (DWORD)http.corpo.size(), 0)
             && WinHttpReceiveResponse(req, nullptr);
    }
    DWORD codice = 0, dim = sizeof(codice);
    if (ok) WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &codice, &dim, nullptr);
    while (ok) {
        DWORD disponibili = 0;
        if (!WinHttpQueryDataAvailable(req, &disponibili) || disponibili == 0) break;
        std::string pezzo(disponibili, '\0');
        DWORD letti = 0;
        if (!WinHttpReadData(req, &pezzo[0], disponibili, &letti)) { ok = false; break; }
        risposta.append(pezzo.data(), letti);
    }
    if (!ok) errore = "connessione non riuscita (codice " + std::to_string(GetLastError()) + "): controlla internet";
    else if (codice != 200) { errore = "il server ha risposto " + std::to_string(codice) + ": " + risposta; ok = false; }
    if (req) WinHttpCloseHandle(req);
    if (conn) WinHttpCloseHandle(conn);
    if (sessione) WinHttpCloseHandle(sessione);
    http.risposta = ok ? risposta : errore;
    InterlockedExchange(&http.stato, ok ? 2 : 3);
    return 0;
}
} // namespace

void HttpAvviaPost(const std::string& host, const std::string& percorso,
                   const std::string& intestazioni, const std::string& corpo) {
    if (http.stato == 1) return;
    http.host = Largo(host);
    http.percorso = Largo(percorso);
    http.intestazioni = Largo(intestazioni);
    http.corpo = corpo;
    http.risposta.clear();
    InterlockedExchange(&http.stato, 1);
    HANDLE t = CreateThread(nullptr, 0, ThreadHttp, nullptr, 0, nullptr);
    if (t) CloseHandle(t);
    else { http.risposta = "impossibile avviare la richiesta"; InterlockedExchange(&http.stato, 3); }
}

int HttpStato() { return (int)http.stato; }
std::string HttpRisposta() { return http.stato >= 2 ? http.risposta : std::string(); }

#else // Linux e altri sistemi: utile per sviluppare, l'obiettivo resta Windows.
#include <unistd.h>
#include <climits>

std::string PercorsoEseguibile() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf));
    return n > 0 ? std::string(buf, (size_t)n) : std::string();
}

static int statoHttp = 0;
void HttpAvviaPost(const std::string&, const std::string&, const std::string&, const std::string&) { statoHttp = 3; }
int HttpStato() { return statoHttp; }
std::string HttpRisposta() { return "il collegamento a Claude e' disponibile nella versione Windows"; }
#endif

std::string CartellaEseguibile() {
    std::string p = PercorsoEseguibile();
    size_t i = p.find_last_of("/\\");
    return i == std::string::npos ? std::string(".") : p.substr(0, i);
}

std::string LeggiVariabileAmbiente(const char* nome) {
    const char* v = std::getenv(nome);
    return v ? std::string(v) : std::string();
}

#ifdef _WIN32
void RendiEseguibile(const std::string&) {}
#else
#include <sys/stat.h>
void RendiEseguibile(const std::string& percorso) { chmod(percorso.c_str(), 0755); }
#endif
