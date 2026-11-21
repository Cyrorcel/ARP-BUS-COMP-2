# ARP BUS COMP (VST3 / AU, macOS)

Compresseur de bus / mastering, façade vintage (images dans `assets/`).

## Compiler (Mac, Apple Silicon ou Intel)

Xcode complet n'est PAS nécessaire. Il faut seulement les outils en ligne de commande
(`xcode-select --install`) et CMake (`brew install cmake`).

    cd ARPBusComp
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j 4

## Sans rien installer : compilation dans le cloud (GitHub Actions, gratuit)

1. Créez un dépôt GitHub (privé ou public) et envoyez-y le contenu de ce dossier
   (y compris `.github/workflows/build.yml`).
2. Onglet "Actions" > "Build VST3 (macOS)" > "Run workflow".
3. Une fois terminé, téléchargez l'artefact `ARP-BUS-COMP-VST3-macOS`, décompressez-le,
   puis placez `ARP BUS COMP.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`.
4. Retirez la quarantaine : `xattr -cr "$HOME/Library/Audio/Plug-Ins/VST3/ARP BUS COMP.vst3"`.

Le plugin est copié automatiquement dans :

- `~/Library/Audio/Plug-Ins/VST3/ARP BUS COMP.vst3`
- `~/Library/Audio/Plug-Ins/Components/ARP BUS COMP.component`

## Reaper

Options > Preferences > Plug-ins > VST > Re-scan (ou "Clear cache and re-scan"). Puis FX > "ARP BUS COMP".

Si macOS bloque le plugin (non signé) : `xattr -cr "~/Library/Audio/Plug-Ins/VST3/ARP BUS COMP.vst3"`.

## Paramètres

Input, Compression, Output, Mix (compression parallèle), Power (bypass). Voir plus bas.

### ARP BUS COMP – fonctionnement

Commandes : **Input** (±12 dB), **Compression** (0 à 40), **Output** (±6 dB), **Mix** (0–100 %), interrupteur **Power** (bypass, voyant rouge).

- **Compression** : tourner vers la droite abaisse le seuil (0 -> seuil 0 dB ; 40 -> seuil -40 dB).
- **Makeup automatique** (sans bouton) : 0 dB à 0, +8 dB à 40, progression linéaire (0,2 dB par point de Compression).
  Voir `autoMakeupDb()` dans `CompCore.h`.
- **Réglages fixes (non visibles)** : Ratio 2:1, Attack 30 ms, Release 30 ms. Il n'y a plus d'étage tube/saturation.
  Constantes `kFixed...` dans `CompCore.h`.
- Façade : `assets/*.png` (rendus @2x ; le corps des boutons tourne, plateau/reflets fixes, aiguille du VU dessinée en code).
