# LiteCGSS
## Description
Le `LiteCGSS` est l'implémentation C++ sur laquelle le `LiteRGSS2` va se baser.

Le but de ce projet est de fournir une version C++ du `LiteRGSS` affin de simplifier le Binding du `LiteRGSS2` et surtout de complètement isoler la partie logique C++ de la partie logique Ruby.

Si vous ne savez pas à quoi correspond le `LiteRGSS`, alors n'hésitez pas à regarder la documentation présente sur le [GitHub LiteRGSS](https://github.com/NuriYuri/LiteRGSS)

Dans les grandes lignes, le `LiteRGSS` est une surcouche SFML qui offre un binding Ruby conforme à ce que propose le RGSS officiel de RPG Maker XP.

## Les principes du LiteRGSS

Qu'est-ce que le `LiteRGSS` possède, que le RGSS officiel n'a pas ?
*  De meilleures performances, à l'aide notamment d'algorithmes compilés directement en C++ et de l'utilisation de SFML
*  Quelques fonctionnalités que le RGSS officiel n'a pas : TODO exemples

Qu'est-ce que le `LiteRGSS` possède, que SFML n'a pas ?
*  Un système de View qui permet de gérer l'appartenance des ressources graphiques à une partie de l'écran
*  Offre une certaine abstraction du processus de dessin à l'écran : avec SFML et SDL on dit explicitement quel sprite/texture est dessiné à quel moment, avec le LiteRGSS on définit juste le sprite et `Graphics.update` se chargera du reste.
*  Certaines classes utilitaires ne sont pas offertes par la SFML, TODO exemples 
*  Un binding Ruby, bien sûr

Exemple d'utilisation des ressources graphiques :
<div style="display: grid;grid-template-columns: 1fr 1fr;grid-gap: 0.25rem;">
<div>
Sous SFML :

```cpp
// Window assumed to be created
sf::Texture texture;
texture.loadFromFile("image.png")
sf::Sprite sprite;
sprite.setTexture(texture);
// Main loop
while(true)
{
    window.clear(sf::Color::Black);
    // Draw other stuff
    // ...
    // Draw the sprite
    window.draw(sprite);
    // Display the result
    window.display();
}
```

</div>
<div>
Sous le LiteRGSS :

```ruby
# Graphics assumed to be initialized
sprite = Sprite.new
sprite.bitmap = Bitmap.new('image.png')
# Main loop
loop do
  Graphics.update
end
```

</div>
</div>

# Pourquoi avoir voulu une refonte : l'architecture

Avant, le binding Ruby et la logique s'entremêlaient dans le même projet, principalement dans le même fichier, mais parfois dans la même fonction.

Nous ne voulons pas reproduire ce schéma dans cette nouvelle version du `LiteRGSS` :
*  La lisibilité était compliquée, en mixant l'API C pour bind le Ruby, la SFML et la logique en elle-même
*  Déterminer l'origine précise d'un bug : l'algorithme, le binding ou encore notre utilisation de la SFML ?
*  Philosophie : le code du LiteRGSS doit être plus simple. C'est parce que le but initial de LiteRGSS est simple que ce code doit être rédigeable / lisible sans aucune difficulté.

Donc, l'idée principale qui a émergé était de séparer le projet `LiteRGSS` en deux parties distinctes, même si cela implique une refonte.

De cette façon, nous tirons des bénéfices :
*  Modularité : le code du `LiteCGSS` pourrait être utilisé en dehors du `LiteRGSS`. Par exemple, on pourrait réaliser un binding Python (mais ce n'est pas actuellement prévu ;)). Ou utiliser uniquement C++.
*  Lisibilité : soit on lit le code du binding (`LiteRGSS`), soit le code du moteur (`LiteCGSS`). Pas de code de binding Ruby au milieu du C++. Pas de code C++ au milieu du binding Ruby.
*  Testabilité : il est plus simple d'isoler les parties de code qui ne dépendent pas du binding Ruby. On peut maintenant avoir une batterie de tests unitaires côté C++ uniquement, ce qui était difficilement possible avec le binding Ruby entremêlé.
*  Traquer les bugs : plus simple, c'est impliqué par la lisibilité et la testabilité.

## Exemples de code en LiteCGSS

### Création d'une fenêtre et attente de la fermeture
```cpp
// Création des settings : fenêtre en 640x480 32bits en windowed sans resize autorisé
cgss::DisplayWindowSettings settings(640, 480, 32, false, false);
// On active le SYNC (40Hz, 60Hz, 144Hz, 240Hz selon moniteur)
settings.vSync = true;

// Création de la fenêtre
cgss::DisplayWindow window(settings, "LiteCGSS");

// Main Loop
while(window.isOpen)
{
    window.update();
}
```

### Création de Sprite et Viewports
```cpp
// Init omitted

// Sprite in main Window
cgss::Sprite& sprite1 = cgss::Sprite.create(window);

// Viewport sprite definition
cgss::Viewport& viewport = cgss::Viewport.create(window, 0, 0, 640, 480);
cgss::Sprite& sprite2 = cgss::Sprite.create(viewport);

// Texture loading
cgss::Texture& texture = cgss::Texture.create(window, "image.png");

// Settings of Sprite1
sprite1.setTexture(texture);

// Settings of Sprite2
sprite2.setTexture(texture);
sprite2.setSrcRectDiv(0, 0, 2, 2);
sprite2.setMirror(true);

// Main Loop
while(window.isOpen)
{
    window.update();
}
```

Question : Pourquoi ne pas utiliser `sf::Texture` ?  
Réponse : Pour pouvoir créer une texture à la manière du `LiteRGSS` et pour pouvoir ajouter des fonctions sans empietter dans le namespace de SFML.

Question : Pourquoi utiliser `.create` ?  
Réponse : La mémoire doit être géré par le `LiteCGSS` de ce fait une fonction statique `create` est disponible pour toute les entités gérées. Ces entités on nécessairement une fonction `dispose` si l'utilisateur a besoin de s'en débarasser avant que le manager s'en charge.  
Le manager (DisplayWindow, Viewport, FramedView) libère automatiquement la mémoire des entités auxquelles il est lié lorsqu'il est détruit.  
L'utilisateur est responsable des `DisplayWindow` puis-ce que c'est les entités de plus haut niveau. (Personne d'autre les gère.)

Question : Pourquoi doit-on définir une fenêtre comme entité à un `Sprite` ou un `Viewport`.
Réponse : Le `LiteCGSS` et `LiteRGSS2` prévoient la possibilité d'afficher plusieurs fenêtre en cas de besoin. Pour le cas des textures, lorsqu'une fenêtre est précisée les settings de la fenêtre sont utilisés pour configurer la texture (smooth).

# Spécifications techniques

Système de build : `CMake`
Framework de test : `GoogleTest`

# Compilation

Avant tout, je vais supposer que vous avez au moins un "git clone" du projet.
Pour mon explication, je vais utiliser le répertoire "D:\LiteCGSS" comme répertoire cloné de ce projet.

Outils requis : 
* CMake
* SFML (version x64 ou x86 selon votre choix) et ses libs dynamiques
* Un compilateur à jour (avec support de C++17)

Concernant SFML, le mieux est de la compiler directement en utilisant CMake. 
Pour cela, il faut télécharger les sources sur le site officiel. Le lien est ici pour la dernière version en date à ce jour : https://www.sfml-dev.org/download/sfml/2.5.1/index-fr.php
Ensuite, voici une page récapitulant les étapes de compilation : https://www.sfml-dev.org/tutorials/2.5/compile-with-cmake-fr.php

Ici, je vais faire un résumé par un exemple sous Windows, en utilisant Visual Studio.
Je prends pour exemple "D:\SFML-2.5.1-x86" pour le répertoire contenant les sources de SFML que je compilerai pour x86.
Ainsi que "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\" comme répertoire vers Visual Studio.

```
D:\SFML-2.5.1-x86> call "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
D:\SFML-2.5.1-x86> del CMakeCache.txt
D:\SFML-2.5.1-x86> cmake -A "Win32" .
-- The C compiler identification is MSVC 19.25.28614.0
-- The CXX compiler identification is MSVC 19.25.28614.0
-- Check for working C compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe
-- Check for working C compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe - works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe
-- Check for working CXX compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe - works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Looking for sys/types.h
-- Looking for sys/types.h - found
-- Looking for stdint.h
-- Looking for stdint.h - found
-- Looking for stddef.h
-- Looking for stddef.h - found
-- Check size of void*
-- Check size of void* - done
-- Found OpenGL: opengl32
-- Found OpenAL: D:/SFML-2.5.1-x86/extlibs/libs-msvc-universal/x86/openal32.lib
CMake Warning (dev) at .../cmake/3.17.1/share/cmake-3.17/Modules/FindPackageHandleStandardArgs.cmake:272 (message):
  The package name passed to `find_package_handle_standard_args` (VORBIS)
  does not match the name of the calling package (Vorbis).  This can lead to
  problems in calling code that expects `find_package` result variables
  (e.g., `_FOUND`) to follow a certain pattern.
Call Stack (most recent call first):
  cmake/Modules/FindVorbis.cmake:25 (find_package_handle_standard_args)
  cmake/Macros.cmake:279 (find_package)
  src/SFML/Audio/CMakeLists.txt:70 (sfml_find_package)
This warning is for project developers.  Use -Wno-dev to suppress it.

-- Found VORBIS: D:/SFML-2.5.1-x86/extlibs/libs-msvc-universal/x86/vorbisenc.lib;D:/SFML-2.5.1-x86/extlibs/libs-msvc-universal/x86/vorbisfile.lib;D:/SFML-2.5.1-x86/extlibs/libs-msvc-universal/x86/vorbis.lib;D:/SFML-2.5.1-x86/extlibs/libs-msvc-universal/x86/ogg.lib        
-- Found FLAC: D:/SFML-2.5.1-x86/extlibs/libs-msvc-universal/x86/flac.lib
-- Configuring done
-- Generating done
-- Build files have been written to: D:/SFML-2.5.1-x86
```

A cette étape, les fichiers nécessaires à la compilation de SFML (en x86 avec MSVC) sont générés.

Etape optionnelle : nous allons maintenant paramétrer SFML afin d'ajuster ses options de compilation.
Pour cela, le plus simple (et le plus abordable) est d'utiliser "cmake-gui" :

```
D:\SFML-2.5.1-x86> cmake-gui
```

Une fenêtre s'ouvre.
Mettre en dossier de code source "D:/SFML-2.5.1-x86" (le dossier contenant le code téléchargé sur le site sfml-dev, dans lequel nous avons déjà généré les fichiers CMake)
Mettre en dossier de sortie pour les binaires le dossier de votre choix ("D:/SFML-2.5.1-x86-out" pour moi par exemple)
Cliquez ensuite sur "Configure".
Il est possible que l'on vous redemande le générateur : sélectionner votre version de Visual Studio.
Dans la partie architecture, renseigner "Win32" pour du x86
Une fois le projet configuré, vous pouvez modifier les valeurs utilisées par SFML.
Je vous conseille notamment de décocher "SFML_BUILD_EXAMPLES" et "SFML_BUILD_DOC".
Cliquez ensuite à nouveau sur "Configure" puis "Generate". Tout se déroule en principe avec succès.
Fin de l'étape optionnelle.

Maintenant, nous allons lancer la compilation de SFML.
```
D:\SFML-2.5.1-x86> cmake --build .
```
La compilation s'effectue et permet d'obtenir en sortie diverses bibliothèques dynamiques, qui seront utilisées entre autre lors du linkage de notre LiteCGSS, mais aussi lors de son exécution.

Placez-vous ensuite dans le répertoire racine du projet LiteCGSS, et effectuez une opération similaire :
```
D:\LiteCGSS> call "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
D:\LiteCGSS> del CMakeCache.txt
D:\LiteCGSS> cmake -A"Win32" .
-- The C compiler identification is MSVC 19.25.28614.0
-- The CXX compiler identification is MSVC 19.25.28614.0
-- Check for working C compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe
-- Check for working C compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe - works
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Detecting C compile features
-- Detecting C compile features - done
-- Check for working CXX compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe
-- Check for working CXX compiler: D:/Program Files (x86)/Microsoft Visual Studio/2019/Community/VC/Tools/MSVC/14.25.28610/bin/Hostx86/x86/cl.exe - works
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Detecting CXX compile features
-- Detecting CXX compile features - done
CMake Error at CMakeLists.txt:16 (find_package):
  By not providing "FindSFML.cmake" in CMAKE_MODULE_PATH this project has
  asked CMake to find a package configuration file provided by "SFML", but
  CMake did not find one.

  Could not find a package configuration file provided by "SFML" (requested
  version 2) with any of the following names:

    SFMLConfig.cmake
    sfml-config.cmake

  Add the installation prefix of "SFML" to CMAKE_PREFIX_PATH or set
  "SFML_DIR" to a directory containing one of the above files.  If "SFML"
  provides a separate development package or SDK, be sure it has been
  installed.


-- Configuring incomplete, errors occurred!
See also "D:/LiteCGSS/CMakeFiles/CMakeOutput.log".
See also "D:/LiteCGSS/CMakeFiles/CMakeError.log".
```
Comme vous pouvez le voir, on nous demande d'expliciter le répertoire d'installation de SFML.
On peut par exemple créer la variable "SFML_DIR" pointant vers le répertoire "D:\SFML-2.5.1-x86".
Le mieux est de directement la modifier au niveau des variables d'environnement.
Une fois la variable renseignée, on peut relancer la configuration :

```
D:\LiteCGSS> cmake .
-- Found SFML 2.5.1 in D:/SFML-2.5.1-x86-out
-- Configuring done
-- Generating done
-- Build files have been written to: D:/LiteCGSS
```
Félicitations, LiteCGSS est correctement configuré et (enfin !) prêt à être compilé !

Nous pouvons terminer en lançant la compilation :
```
D:\LiteCGSS> cmake --build .
```
Tout devrait maintenant compiler avec succès.
Maintenant, concernant l'exécution des binaires utilisant le LiteCGSS : cela nécessite les libs dynamiques créées lors de la compilation de SFML.
Le plus simple est donc de copier / coller les DLL contenues dans le répertoire "D:\SFML-2.5.1-x86\lib\Debug" pour une target "Debug" ou simplement "D:\SFML-2.5.1-x86\lib" pour une target "Release" et de les placer à côté des exécutables.
