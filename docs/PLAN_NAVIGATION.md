# Draille — Plan : Import GPX + Navigation guidée

## Vue d'ensemble

```
[Import WiFi] → [Parser GPX] → [RouteManager] → [NavigationEngine] → [Écran NAVIGATE]
```

5 phases indépendantes, livrables l'une après l'autre.

---

## Contraintes hardware

| Ressource | Disponible | Besoin navigation |
|---|---|---|
| Flash SPIFFS | ~1.5 MB | Routes GPX stockées (~50 KB/h de trace) |
| RAM interne | 320 KB | Structs légers, state machine |
| PSRAM | 8 MB | Array de RoutePoints complet en mémoire |
| GPS | NEO-M10S | Lat/lon, heading, vitesse — tout ce qu'il faut |
| Affichage | 240×240 rond | Compas, flèche, métriques |

---

## Phase A — Import GPX via WiFi

**Complexité : faible**

Ajout d'un endpoint `POST /import` au serveur WiFi existant.

### Ce qui change

- `WifiExport.h/.cpp` : ajout route `POST /import`
- `TrackStorage.h/.cpp` : ajout `saveRoute(filename, data, size)` + index des routes séparé de l'index des sessions (`/routes.json`)

### Flux utilisateur

```
1. Long press SELECT → WiFi Export s'active
2. Navigateur : POST http://192.168.4.1/import  (multipart ou raw body)
3. ESP32 reçoit le fichier GPX en streaming → l'écrit dans /routes/YYYYMMDD.gpx
4. Confirmation JSON : { "ok": true, "filename": "/routes/..." }
```

### Endpoint

```
POST /import
Content-Type: application/octet-stream
Body: contenu brut du fichier .gpx

→ 200 { "ok": true, "file": "/routes/20240308_083000.gpx" }
→ 413 si fichier > 500 KB
→ 500 si SPIFFS plein
```

### Structure SPIFFS après

```
/tracks/   — sessions enregistrées (existant)
/routes/   — traces importées pour navigation (nouveau)
/sessions.json
/routes.json   — nouveau
```

---

## Phase B — Parser GPX

**Complexité : moyenne**

GPX est du XML. Pas de lib XML standard sur ESP32 → parser SAX custom (machine à états), streaming depuis SPIFFS. Consommation mémoire : ~200 octets de pile d'état.

### Nouveaux fichiers

```
src/nav/GpxParser.h/.cpp
```

### Ce qu'on extrait

```xml
<!-- Route : points à suivre dans l'ordre -->
<rte>
  <rtept lat="44.280" lon="4.720"><ele>85.0</ele></rtept>
</rte>

<!-- Track : trace enregistrée, réutilisable pour navigation -->
<trk>
  <trkseg>
    <trkpt lat="44.281" lon="4.721"><ele>86.0</ele></trkpt>
  </trkseg>
</trk>
```

On traite `<rtept>` et `<trkpt>` de la même façon — les deux deviennent des `RoutePoint`.

### Data model

```cpp
struct RoutePoint {
    double lat;       // 8 bytes
    double lon;       // 8 bytes
    float  elevation; // 4 bytes
};                    // 20 bytes/point

// 2000 points × 20 bytes = 40 KB en PSRAM → 1h30 de trace à 1pt/5s
```

### API du parser

```cpp
class GpxParser {
public:
    // Parse le fichier, remplit le buffer alloué en PSRAM
    bool parse(const char* filepath,
               RoutePoint* buffer, uint16_t maxPoints,
               uint16_t& outCount, float& outTotalKm);
};
```

### Machine à états SAX

```
IDLE → IN_RTE/IN_TRK → IN_POINT → IN_ELE → back
       (tag <rtept>/<trkpt> détecté)
       (attributs lat/lon lus directement)
```

---

## Phase C — Navigation Engine

**Complexité : moyenne-forte**

Calcul géographique pur. Pas de dépendance hardware — testable nativement.

### Nouveaux fichiers

```
src/nav/RouteManager.h/.cpp    — chargement + gestion de la route active
src/nav/NavigationEngine.h/.cpp — calculs, état de navigation
```

### RouteManager

```cpp
class RouteManager {
public:
    bool   loadRoute(const char* filepath);  // alloue en PSRAM via ps_malloc
    void   unloadRoute();
    bool   isLoaded() const;
    uint16_t    pointCount() const;
    RoutePoint  getPoint(uint16_t index) const;
    float       totalDistanceKm() const;
};
```

### NavigationEngine — état

```cpp
struct NavState {
    uint16_t targetIdx;        // index du prochain waypoint
    float    crossTrackM;      // mètres, + = droite, - = gauche
    float    bearingDeg;       // cap vers le prochain waypoint (0-360°)
    float    distToNextKm;     // distance au prochain waypoint
    float    distRemainingKm;  // distance totale restante
    bool     offTrack;         // true si cross-track > OFF_TRACK_THRESHOLD
    bool     arrived;          // true si dernier waypoint atteint
};
```

### Algorithmes

**Bearing A → B :**
```
θ = atan2(sin(Δlon)·cos(lat2),
          cos(lat1)·sin(lat2) − sin(lat1)·cos(lat2)·cos(Δlon))
```

**Cross-track error (distance perpendiculaire à la trace) :**
```
XTD = asin(sin(d_AP / R) · sin(θ_AP − θ_AB)) · R

où :
  d_AP  = distance GPS actuel → point A (début segment)
  θ_AP  = bearing GPS actuel → A
  θ_AB  = bearing A → B (segment cible)
  R     = 6371 km
```

**Avancement au waypoint suivant :**
```
si distance au waypoint cible < 15m → targetIdx++
```

**Seuil off-track :** `|crossTrackM| > 30m`

### API

```cpp
class NavigationEngine {
public:
    void      setRoute(RouteManager& route);
    NavState  update(double lat, double lon);  // appelé à chaque fix GPS
    void      reset();
};
```

---

## Phase D — Écran NAVIGATE

**Complexité : moyenne**

Nouveau screen dans `DisplayManager`. Conçu pour être lu en mouvement, en un coup d'œil.

### Layout (240×240 rond)

```
        ┌────────────────────────┐
       /   ╔══════════╗           \
      │    ║    ↑     ║            │  ← flèche de direction (rotation selon bearing)
      │    ║  NNO     ║            │
      │    ╚══════════╝            │
      │                            │
      │     ← 12 m                 │  ← cross-track bar (centré = sur la trace)
      │  ━━━━━━━━━━█━━━━━━━━━━━━  │    rouge si off-track
      │                            │
      │        0.8 km              │  ← distance au prochain waypoint
      │       12.4 km restants     │  ← distance totale restante
       \                          /
        └────────────────────────┘
```

**Cross-track bar :**
- Barre horizontale centrée
- Un curseur se déplace gauche/droite selon `crossTrackM`
- Plage : ±50m = pleine largeur
- Couleur : blanc < 15m / amber 15-30m / rouge > 30m

**Flèche de direction :**
- Calculée : `bearing - currentHeading` (heading GPS quand vitesse > 2 km/h)
- Rotation dessinée en sprites

### Nouveau data struct

```cpp
struct NavigateData {
    float    relBearingDeg;   // bearing relatif à la direction actuelle
    float    crossTrackM;     // mètres, signe = gauche/droite
    float    distToNextKm;
    float    distRemainingKm;
    bool     offTrack;
    bool     arrived;
};
```

---

## Phase E — Intégration UI

**Complexité : faible**

### Flux utilisateur

```
Écran TRACKS
  → PREV/NEXT : défiler les routes importées (et sessions)
  → SELECT court : voir détails
  → SELECT long (2s) : démarrer navigation sur cette route
    → bascule sur écran NAVIGATE
    → PREV/NEXT désactivés pendant navigation
    → SELECT court : stop navigation → retour DASHBOARD
```

### Ce qui change dans UIManager

- Nouveau `AppScreen::NAVIGATE`
- `_onLongSelect()` sur TRACKS : `NavigationEngine.setRoute()` + changer screen
- `_wireNavigation()` : à chaque update GPS → `NavEngine.update(lat, lon)` → refresh écran si NAVIGATE actif

---

## Résumé des nouveaux fichiers

```
src/
└── nav/
    ├── GpxParser.h/.cpp         — parser SAX streaming
    ├── RouteManager.h/.cpp      — chargement PSRAM, accès points
    └── NavigationEngine.h/.cpp  — calculs bearing / XTD / state
```

Modifications :
- `WifiExport.h/.cpp`    — endpoint POST /import
- `TrackStorage.h/.cpp`  — gestion /routes/ + routes.json
- `DisplayManager.h/.cpp` — drawNavigate()
- `UIManager.h/.cpp`     — AppScreen::NAVIGATE + wiring

---

## Tests unitaires natifs (Phase C)

La navigation engine est pure math → testable sans hardware.

```
test/native/test_navigation/
├── test_bearing.cpp      — bearing entre coords connues
├── test_cross_track.cpp  — XTD sur segments simples
└── test_waypoint_adv.cpp — logique d'avancement de waypoint
```

---

## Ordre de livraison recommandé

```
Phase A (Import WiFi) → Phase B (Parser GPX) → Phase C (Nav Engine + tests)
                                                     ↓
                                         Phase D (Écran) + Phase E (UI)
```

Phase C est le cœur — livrer ses tests unitaires avant de toucher au hardware.
