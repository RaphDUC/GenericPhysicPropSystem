# GenericPhysicPropSystem Plugin

## English Version

### Overview
**GenericPhysicPropSystem** is a robust and lightweight plugin designed to handle interactive physics props for Game Jams. Oriented towards a **"Source Engine"** feel, it provides out-of-the-box solutions for:
- **Optimization:** Automatically puts distant objects to sleep and manages tick rates.
- **Audio Feedback:** Plays impact sounds based on the surface material (Physical Material) and impact velocity.
- **Interaction:** built-in `Grab`, `Drop`, and `Throw` mechanics with anti-clipping protection.
- **Responsive Physics:** Objects react dynamically to damage (AnyDamage, PointDamage, RadialDamage) with configurable impulses.

### Dependencies
- **GenericDamageSystemPlugin**: Used to handle generic damage types and impulse modifiers.
- **PhysicsCore**: Standard Unreal physics module.

### Core Classes

#### 1. UPhysicsPropComponent
The heart of the system. Add this component to any Actor you want to turn into a physics prop.

**Key Features & Properties:**
- **Optimization:**
  - `PhysicsCullDistance`: Distance (cm) at which the object stops ticking or simulating physics to save performance.
  - `DistanceCheckInterval`: How often to check the distance to the player.
- **Collision & CCD:**
  - `CCDSpeedThreshold`: Activates Continuous Collision Detection if the object moves faster than this threshold (prevents tunneling through walls).
- **Audio:**
  - `ImpactTable`: Reference to a `UPropPhysicsImpactData` Data Asset.
  - `MinImpactThreshold`: Minimum force required to play a sound.
  - `ImpactCooldown`: Prevents audio spam (e.g., rolling objects).
- **Interaction:**
  - `Grab(USceneComponent* Holder)`: Picks up the object. Includes logic to avoid clipping into walls.
  - `Throw(FVector Direction, float Force)`: Launches the object.
  - `Drop()`: Releases the object preserving inertia.
- **Damage Reaction:**
  - Compatible with `UPhysicsPropDamageType` for specific tuning.
  - Compatible with `UGenericDamageType` (from GenericDamageSystemPlugin) to use global `ImpulseModifier`.

#### 2. UPropPhysicsImpactData (Data Asset)
A Data Asset used to map `UPhysicalMaterial` to specific sounds.
- **ImpactMap**: Assigns a SoundBase and Volume Multiplier to a Physical Material (e.g., Wood -> WoodThud.wav).
- **DefaultSound**: Fallback sound if the material is unknown.

#### 3. UPhysicsPropDamageType
A custom DamageType that allows designers to tweak physics reactions without changing the damage amount.
- `ImpulsePower`: Multiplier for the force applied to the prop upon impact.
- `bForceWake`: Allows waking up "sleeping" props instantly when damaged.

### Usage Guide

#### Step 1: Setting up a Prop
1. Create a Blueprint Actor (e.g., `BP_PhysBox`).
2. Add a `StaticMeshComponent` and set it as Root.
3. Add the **`PhysicsPropComponent`**.
4. (Optional) Call `Physicalize` in BeginPlay if you want to force initialization on a specific mesh, otherwise it automatically finds the first StaticMesh.

#### Step 2: Configuring Audio
1. Right-click in Content Browser -> **Data Asset** -> select **`PropPhysicsImpactData`**.
2. Assign generic impact sounds (Default Sound).
3. Add entries for specific materials (e.g., if your floor has a `PM_Concrete` Physical Material, assign a concrete impact sound).
4. Assign this Data Asset to the `ImpactTable` property of your `PhysicsPropComponent`.

#### Step 3: Interaction (Blueprint)
In your Character Blueprint:
1. When pressing the Interact key, raycast to find an Actor.
2. Get the `PhysicsPropComponent` from the hit actor.
3. Call `Grab(HeldObjectPosition)` where `HeldObjectPosition` is a SceneComponent attached to your camera.
4. Call `Throw` or `Drop` to release it.

---

## Version Française

### Vue d'ensemble
**GenericPhysicPropSystem** est un plugin robuste et léger conçu pour gérer les objets physiques interactifs pour les Game Jams. Inspiré par la physique du **"Source Engine"**, il fournit des solutions prêtes à l'emploi pour :
- **L'Optimisation :** Endort automatiquement les objets lointains et gère la fréquence de tick.
- **Le Feedback Audio :** Joue des sons d'impact basés sur le matériau de surface (Physical Material) et la vitesse d'impact.
- **L'Interaction :** Mécaniques intégrées de `Grab` (Saisir), `Drop` (Lâcher) et `Throw` (Lancer) avec protection anti-clipping (traversée de murs).
- **La Physique Réactive :** Les objets réagissent dynamiquement aux dégâts (AnyDamage, PointDamage, RadialDamage) avec des impulsions configurables.

### Dépendances
- **GenericDamageSystemPlugin** : Utilisé pour gérer les types de dégâts génériques et les modificateurs d'impulsion.
- **PhysicsCore** : Module physique standard d'Unreal.

### Classes Principales

#### 1. UPhysicsPropComponent
Le cœur du système. Ajoutez ce composant à n'importe quel Acteur pour le transformer en objet physique interactif.

**Fonctionnalités & Propriétés Clés :**
- **Optimisation :**
  - `PhysicsCullDistance` : Distance (cm) à laquelle l'objet arrête de tick ou de simuler la physique pour économiser les performances.
  - `DistanceCheckInterval` : Fréquence de vérification de la distance au joueur.
- **Collision & CCD :**
  - `CCDSpeedThreshold` : Active la détection de collision continue (CCD) si l'objet va plus vite que ce seuil (évite de traverser les murs).
- **Audio :**
  - `ImpactTable` : Référence vers un Data Asset `UPropPhysicsImpactData`.
  - `MinImpactThreshold` : Force minimale requise pour jouer un son.
  - `ImpactCooldown` : Empêche le spam audio (ex: objets qui roulent).
- **Interaction :**
  - `Grab(USceneComponent* Holder)` : Saisit l'objet. Inclut une logique pour éviter de rentrer dans les murs.
  - `Throw(FVector Direction, float Force)` : Lance l'objet.
  - `Drop()` : Relâche l'objet en conservant son inertie.
- **Réaction aux Dégâts :**
  - Compatible avec `UPhysicsPropDamageType` pour des réglages spécifiques.
  - Compatible avec `UGenericDamageType` (du plugin GenericDamageSystem) pour utiliser le `ImpulseModifier` global.

#### 2. UPropPhysicsImpactData (Data Asset)
Un Data Asset utilisé pour associer des `UPhysicalMaterial` à des sons spécifiques.
- **ImpactMap** : Associe un SoundBase et un Multiplicateur de Volume à un Matériau Physique (ex: Bois -> WoodThud.wav).
- **DefaultSound** : Son par défaut si le matériau est inconnu.

#### 3. UPhysicsPropDamageType
Un DamageType personnalisé qui permet aux designers d'ajuster les réactions physiques sans changer le montant des dégâts.
- `ImpulsePower` : Multiplicateur de la force appliquée à l'objet lors de l'impact.
- `bForceWake` : Permet de réveiller instantanément les objets "endormis" lorsqu'ils sont endommagés.

### Guide d'Utilisation

#### Étape 1 : Configurer un Prop
1. Créez un Blueprint Actor (ex: `BP_PhysBox`).
2. Ajoutez un `StaticMeshComponent` et définissez-le comme Racine (Root).
3. Ajoutez le **`PhysicsPropComponent`**.
4. (Optionnel) Appelez `Physicalize` dans le BeginPlay si vous voulez forcer l'initialisation sur un mesh spécifique, sinon il trouve automatiquement le premier StaticMesh.

#### Étape 2 : Configurer l'Audio
1. Clic droit dans le Content Browser -> **Data Asset** -> sélectionnez **`PropPhysicsImpactData`**.
2. Assignez des sons d'impact génériques (Default Sound).
3. Ajoutez des entrées pour des matériaux spécifiques (ex: si votre sol a un matériau physique `PM_Concrete`, assignez un son d'impact de béton).
4. Assignez ce Data Asset à la propriété `ImpactTable` de votre `PhysicsPropComponent`.

#### Étape 3 : Interaction (Blueprint)
Dans votre Blueprint de Personnage :
1. Lors de l'appui sur la touche Interagir, faites un Raycast pour trouver un Acteur.
2. Récupérez le `PhysicsPropComponent` de l'acteur touché.
3. Appelez `Grab(HeldObjectPosition)` où `HeldObjectPosition` est un SceneComponent attaché devant votre caméra.
4. Appelez `Throw` ou `Drop` pour le relâcher.
