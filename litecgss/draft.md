# LiteCGSS
## Description
Le `LiteCGSS` est l'implémentation C++ sur laquelle le `LiteRGSS2` va se baser. Le but de ce projet est de fournir une version C++ du `LiteRGSS` affin de simplifier le Binding du `LiteRGSS2` et surtout de complètement isoler la partie logique C++ de la partie logique Ruby.

## Les principes du LiteRGSS

Contrairement à un binding classique de SFML ou SDL, le LiteRGSS offre une certaine abstraction du processus de dessin à l'écran. Avec SFML et SDL on dit explicitement quel sprite/texture est dessiné à quel moment, avec le LiteRGSS on définit juste le sprite et `Graphics.update` se chargera du reste.

Exemple :
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

## Fonctionnement attendu du LiteCGSS

* Une fenêtre (DisplayWindow) permet d'afficher les sprites, textes et viewport qui lui sont lié.
* Un Viewport permet d'afficher les sprites, textes, shape et window qui lui sont lié.
* Une Window (FramedView) permet d'afficher les sprites, textes et shape qui lui sont lié.
* Un élément graphique ne peut pas changer de parent (window / viewport / fenêtre). Il est donc initialisé avec pour premier paramètre le parent auquel il sera lié.
* Un élément graphique est responsable de se délier de son parent **si et seulement si** il y est toujours lié.
* Un élément de type View libère tous les éléments graphiques s'il est libéré **et avant** de libérer les éléments graphiques il les délie.  
    Délier les éléments graphiques avant de les libérer a pour but d'optimiser l'opération (car on ne dira que à l'élément graphique qu'il n'est plus lié, on ne dira pas à l'élément view qu'il n'est plus lié car cette opération ne sert à rien).
* Tous les éléments de type View sont responsable de remettre la View à l'état dans lequel elle était avant de la changer. (Viewport, FramedView)
* Une fenêtre possède du z-ordering
* Les view ne possèdent pas de z-ordering sauf si c'est demandé
* Les évènements sont traités à l'aide de "callbacks".
* Le LiteCGSS n'est pas responsable de la détection des inputs (SFML le fait très bien, il suffit de bind le bon callback pour récupérer les inputs désirés dans la boucle d'évènement).
* Le LiteCGSS n'est pas responsable des Audios (l'utilisateur doit choisir lui même s'il préfère FMOD ou ce qui est embarqué dans SFML).

## Exemples de code en LiteCGSS

### Création d'une fenêtre et attente de la fermeture
```cpp
// Création des settings : fenêtre en 640x480 32bits en windowed sans resize autorisé
LiteCGSS::DisplayWindowSettings settings(640, 480, 32, false, false);
// On active le SYNC (40Hz, 60Hz, 144Hz, 240Hz selon moniteur)
settings.vsync = true;
// Pour les applications basés sur les frames, on active le frame balancing à 60FPS
// Ca permet de skip des frames ou d'en dessiner plus si besoin
// 0 = pas de fps balancing
settings.fpsBalancingRate = 60;
// Pour indiquer si la fenêtre attend le focus pour dessiner les frames (blocage de la fonction update)
settings.waitFocusToDraw = true;

// Création de la fenêtre
LiteCGSS::DisplayWindow window(settings, "LiteCGSS");

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
LiteCGSS::Sprite& sprite1 = LiteCGSS::Sprite.create(window);

// Viewport sprite definition
LiteCGSS::Viewport& viewport = LiteCGSS::Viewport.create(window, 0, 0, 640, 480);
LiteCGSS::Sprite& sprite2 = LiteCGSS::Sprite.create(viewport);

// Texture loading
LiteCGSS::Texture& texture = LiteCGSS::Texture.create(window, "image.png");

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

## Les différentes classes du LiteCGSS
### `LiteCGSS::DisplayWindowSettings`

Définit les configuration de la DisplayWindow qui sera instancié avec.  
Origine : [sf::VideoMode](https://www.sfml-dev.org/documentation/2.5.1/classsf_1_1VideoMode.php#ac1be160a4342e6eafb2cb0e8c9b18d44)

Propriétés :
```cpp
uint32_t width; // Largeur de la fenêtre (avant scale)
uint32_t height; // Hauteur de la fenêtre (avant scale)
uint32_t bitsPerPixel; // Nombre de bit par pixel
float scale; // Opération de scale des dimensions de la fenêtre
bool fullScreen; // Si la fenêtre sera en fullscreen
bool vsync; // Si la fenêtre utilisa la synchronisation verticale
bool resizable; // Si la fenêtre peut être redimensionné (impossible avec le fullscreen)
bool waitFocusToDraw; // Si la fonction update s'arrête tant que la fenêtre n'a pas le focus
uint32_t fpsBalancingRate; // Framerate cible du jeu s'il est basé sur les frames. 0 = non géré. Si le jeu fini par mettre plus de 100ms à traiter une frame, le fps balancing s'arrête (équivalent à 0).
std::string icon; // Chemin ou contenu du fichier utilisé en icone. Le contenu est détecté si la chaine commence par "\x89PNG"
```

Fonction publique :
```cpp
bool isValid()
```

Fonctions Statiques :
```cpp
sf::VideoMode getDesktopMode();
const std::vector<sf::VideoMode>& getFullscreenModes();
```

---
TODO : Définir les autres classes

---

## Procédure de z-ordering

Lorsque la propriété z d'un élément graphique est affecté (uniquement à ce moment, lors de la création de l'élément on met l'élément à la fin du stack même si z = 0 par défaut) :
* Vérifier si la propriété z a été modifiée ou vaut 0
    * Si oui, Vérifier si le Z-Ordering est autorisé sur l'objet parent
        * Si oui, déclencher le z-ordering de l'élément

Lorsque l'on traite le tri, les étapes sont les suivantes :
* Itérer jusqu'à trouver un élément de z >= et d'index >
    * Pendant cette procédure lever un flag détecté si le élément dont le z change a été trouvé
* Si le flag est levé :
    * Insérer l'élément à la position précédent (swap) et déplacer tous les éléments jusqu'à la position retrouver l'élément
* Sinon :
    * Insérer l'élément à la position actuelle et déplacer les éléments jusqu'à trouver l'élément.

Exemple :
```ruby
# stack (* = notre sprite, nombre = coordonnée z.index)
stack = [0.0, 0.15, 1.1, 1.2, 2.33, 3.55, *, 4.66, 5.3, 6.4, 0.5, 0.6, 0.7]

# Changement 1
sprite.z = 2 # index = 22
swap_el = sprite
# itération
sp_trouve = 2.33
flag = false

# Déplacements
swap_el = 2.33 # 33 > 22
stack = [0.0, 0.15, 1.1, 1.2, *, 3.55, *, 4.66, 5.3, 6.4, 0.5, 0.6, 0.7]
swap_el = 3.55
stack = [0.0, 0.15, 1.1, 1.2, *, 2.33, *, 4.66, 5.3, 6.4, 0.5, 0.6, 0.7]
swap_el = * # /!\
stack = [0.0, 0.15, 1.1, 1.2, *, 2.33, 3.55, 4.66, 5.3, 6.4, 0.5, 0.6, 0.7]

# Changement 2
sprite.z = 3
swap_el = sprite
# itérations
sp_trouve = 3.55 # 55 > 22
flag = true

# Déplacements
swap_el = 2.33
stack = [0.0, 0.15, 1.1, 1.2, *, *, 3.55, 4.66, 5.3, 6.4, 0.5, 0.6, 0.7]
swap_el = * # /!\
stack = [0.0, 0.15, 1.1, 1.2, 2.33, *, 3.55, 4.66, 5.3, 6.4, 0.5, 0.6, 0.7]
```

D'autres implémentations peuvent être faites (exemple stocker l'index réel de l'élément du sprite et réaliser les déplacements dans la bonne direction tant que les conditions ne sont pas respectées).