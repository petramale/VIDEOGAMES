// Conversioni JSON dei singoli elementi della libreria del gioco (implementate in progetto_gioco.cpp).
// Servono all'officina (esporta/importa un elemento) e a Claude (crea e modifica elementi).
#pragma once
#include "progetto_gioco.h"
#include "json.hpp"

nlohmann::json DisegnoJson(const Disegno& d);
bool DisegnoDaJson(const nlohmann::json& j, Disegno& d, std::string& errore);
nlohmann::json PersonaggioJson(const PersonaggioDef& p);
bool PersonaggioDaJson(const nlohmann::json& j, PersonaggioDef& p, std::string& errore);
nlohmann::json OggettoJson(const OggettoDef& o);
bool OggettoDaJson(const nlohmann::json& j, OggettoDef& o, std::string& errore);
nlohmann::json StratoJson(const StratoSfondo& s);
StratoSfondo StratoDaJson(const nlohmann::json& js);
nlohmann::json ScenarioJson(const Scenario& s);
bool ScenarioDaJson(const nlohmann::json& j, Scenario& s, std::string& errore);
nlohmann::json BranoJson(const Brano& b);
bool BranoDaJson(const nlohmann::json& j, Brano& b, std::string& errore);
nlohmann::json EffettoJson(const EffettoSuono& e);
bool EffettoDaJson(const nlohmann::json& j, EffettoSuono& e, std::string& errore);
nlohmann::json RegolaJson(const Regola& r);
bool RegolaDaJson(const nlohmann::json& j, Regola& r, std::string& errore);
