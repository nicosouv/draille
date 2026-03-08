# Draille — Font Setup

## Font used: Inter

Inter est une police open source conçue pour la lisibilité sur écran.
Licence SIL Open Font License — usage libre.

## Étapes (une seule fois)

### 1. Télécharger Inter

[https://fonts.google.com/specimen/Inter](https://fonts.google.com/specimen/Inter)

Extraire les fichiers dans `fonts/` à la racine du projet :

```
fonts/
├── Inter-Regular.ttf
├── Inter-SemiBold.ttf
└── Inter-Bold.ttf
```

### 2. Installer la dépendance Python

```bash
pip install freetype-py
```

### 3. Générer les fichiers `.vlw`

```bash
python3 tools/gen_fonts.py
```

Cela crée dans `data/` :

```
data/
├── Inter_Bold_48.vlw       (~38 KB)
├── Inter_SemiBold_26.vlw   (~22 KB)
└── Inter_Regular_16.vlw    (~16 KB)
```

Total : ~76 KB sur les ~1.5 MB disponibles en SPIFFS.

### 4. Flasher sur le device

```bash
make fs-flash
```

## Comportement sans fonts

Si les fichiers `.vlw` sont absents du SPIFFS, `DisplayManager` bascule automatiquement sur les fonts built-in de TFT_eSPI (non anti-aliasées). Le firmware fonctionne mais l'affichage est moins raffiné.

## Tailles utilisées

| Fichier | Taille | Usage |
|---|---|---|
| `Inter_Bold_48.vlw` | 48 px | Heure, vitesse, métrique principale |
| `Inter_SemiBold_26.vlw` | 26 px | Distance, coordonnées |
| `Inter_Regular_16.vlw` | 16 px | Labels, légendes, statuts |
