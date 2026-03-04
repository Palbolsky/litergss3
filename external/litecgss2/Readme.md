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

# Technical

Build system: `CMake`
Test framework: `GoogleTest`

## Get the required toolchain (CMake + git + SFML binaries)

__CMake__

`scoop install cmake`, or another method to install CMake and get it in your path.

__Git__

`scoop install git`, or another method to install git and get it in your path.

__SFML__

Two choices here:
- You compile SFML from the sources.
After that, you can provide a "SFML_DIR" variable in your environment variables to locate the SFML root dir and make it detected by your system
- You use precompiled SFML binaries.
	- On Windows: if you use mingw compatible env like msys2, you can use `pacman -S mingw-w64-i686-sfml` to install it.
	- On Linux: `sudo apt install libsfml-dev` should do the trick
	- On MacOS: there should be some SFML dev package that you can install with `homebrew`

## Get projects from Gitlab

- Extract this actual repository of LiteCGSS with `git clone https://gitlab.com/NuriYuri/litecgss.git` into the directory of your choice

- LiteRGSS2 automatically embeds skalog as git submodule. To keep everything functional, you have to init all submodules recursively by typing
		`git submodule update --recursive --init`

## Build the LiteCGSS

__Compilation__

Two ways:

### Using CMake

Go back to LiteCGSS root folder.
	Configure your project with
		`cmake -G $YOUR_CMAKE_GENERATOR .`

_Extra options (CMake variables):_

`LITECGSS_NO_TEST`: Toggle (True/False) no-test builds

`LITECGSS_USE_PHYSFS`: Build the assets management with PhysFS lib if it's defined

`CGSS_NO_LOGS`: Logs won't show anywhere if it's defined


Example of a valid CMake configuration:

`cmake -G "MinGW Makefiles" -DLITECGSS_NO_TEST=True -DCGSS_NO_LOGS=True .`

Then, build your project by typing
		`cmake --build .`

### Using Rake

Ensure you have the Ruby dev kit toolchain by typing `ridk enable`

Go back to LiteCGSS root folder. First you have to configure (only the first time you build) the whole project. For that, type `rake configure`

This time we will use the ruby rake compiler toolchain, by simply typing
		`rake clean; rake compile`

_Extra options (Environment variables):_

`LITECGSS_CMAKE_GENERATOR`: allow you to select a specific CMake Generator if defined