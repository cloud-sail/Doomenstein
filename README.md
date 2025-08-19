# Doomenstein
Become the Doom Slayer, relentlessly hunting demons that hide in the shadows. 
Demons run away fast and hide behind the wall, but
- you can use a long range pistol to stagger them (weapon 1)
- you can use a brutal melee attack for instant glory kills (weapon 2)
- you can use radar to locate them (weapon 3)

## Features
- 3D First Person Shooter
- Billboard Sprites
- Flow Field Based AI
- Fast voxel raycasting
- Data-driven actors, weapons and maps
- Sky box

## Gallery
> Weapons   
> ![](Docs/weapon.gif)

> Coward Enemy AI  
> ![](Docs/ai.gif)

## Controls
Mouse and KeyBoard:
Mouse  - Aim
WASD   - Move
Shift  - Walk
1,2,3  - Pistol, Melee, Radar
LMB    - Fire
RMB    - Lock on a demon

GamePad:
Right Stick  - Aim
Left Stick   - Move
L Trigger/A  - Walk
X,Y,B        - Pistol, Melee, Radar
R Trigger    - Fire
L Shoulder   - Lock on a demon

## Data Driven
Actors, Maps, Weapons, Map Tiles are data-driven so that we can easily tweak the stats to have a better player experience.
Definition files are in `Run/Data/Definitions`

Here is an sample for Demon actor:
```xml
<Definitions>
  <!-- Demon -->
  <ActorDefinition name="Demon" faction="Demon" health="160" canBePossessed="true" corpseLifetime="1.15" visible="true">
    <Collision radius="0.35" height="0.85" collidesWithWorld="true" collidesWithActors="true"/>
    <Physics simulated="true" walkSpeed="2.0f" runSpeed="7.5f" turnSpeed="360.0f" drag="9.0f"/>
    <Camera eyeHeight="0.5f" cameraFOV="120.0f"/>
    <AI aiEnabled="true" sightRadius="64.0" sightAngle="120.0" staggerSeconds="3.0" staggerDamageThreshold="40.0"/>
    <Visuals size="2.1,2.1" pivot="0.5,0.0" billboardType="WorldUpFacing" renderLit="true" renderRounded="true" shader="Data/Shaders/Diffuse" spriteSheet="Data/Images/Actor_Pinky_8x9.png" cellCount="8,9">
      <AnimationGroup name="Walk" scaleBySpeed="true" secondsPerFrame="0.25" playbackMode="Loop">
        <Direction vector="-1,0,0"><Animation startFrame="0" endFrame="3"/></Direction>
        <Direction vector="-1,-1,0"><Animation startFrame="8" endFrame="11"/></Direction>
        <Direction vector="0,-1,0"><Animation startFrame="16" endFrame="19"/></Direction>
        <Direction vector="1,-1,0"><Animation startFrame="24" endFrame="27"/></Direction>
        <Direction vector="1,0,0"><Animation startFrame="32" endFrame="35"/></Direction>
        <Direction vector="1,1,0"><Animation startFrame="40" endFrame="43"/></Direction>
        <Direction vector="0,1,0"><Animation startFrame="48" endFrame="51"/></Direction>
        <Direction vector="-1,1,0"><Animation startFrame="56" endFrame="59"/></Direction>
      </AnimationGroup>
      <AnimationGroup name="Attack" secondsPerFrame="0.25" playbackMode="Once">
        <Direction vector="-1,0,0"><Animation startFrame="4" endFrame="6"/></Direction>
        <Direction vector="-1,-1,0"><Animation startFrame="12" endFrame="14"/></Direction>
        <Direction vector="0,-1,0"><Animation startFrame="20" endFrame="22"/></Direction>
        <Direction vector="1,-1,0"><Animation startFrame="28" endFrame="30"/></Direction>
        <Direction vector="1,0,0"><Animation startFrame="36" endFrame="38"/></Direction>
        <Direction vector="1,1,0"><Animation startFrame="44" endFrame="46"/></Direction>
        <Direction vector="0,1,0"><Animation startFrame="52" endFrame="54"/></Direction>
        <Direction vector="-1,1,0"><Animation startFrame="60" endFrame="62"/></Direction>
      </AnimationGroup>
      <AnimationGroup name="Hurt" secondsPerFrame="0.75" playbackMode="Once">
        <Direction vector="-1,0,0"><Animation startFrame="7" endFrame="7"/></Direction>
        <Direction vector="-1,-1,0"><Animation startFrame="15" endFrame="15"/></Direction>
        <Direction vector="0,-1,0"><Animation startFrame="23" endFrame="23"/></Direction>
        <Direction vector="1,-1,0"><Animation startFrame="31" endFrame="31"/></Direction>
        <Direction vector="1,0,0"><Animation startFrame="39" endFrame="39"/></Direction>
        <Direction vector="1,1,0"><Animation startFrame="47" endFrame="47"/></Direction>
        <Direction vector="0,1,0"><Animation startFrame="55" endFrame="55"/></Direction>
        <Direction vector="-1,1,0"><Animation startFrame="63" endFrame="63"/></Direction>
      </AnimationGroup>
      <AnimationGroup name="Death" secondsPerFrame="0.2" playbackMode="Once">
        <Direction vector="1,0,0"><Animation startFrame="64" endFrame="69"/></Direction>
      </AnimationGroup>
    </Visuals>
    <Sounds>
      <Sound sound="Hurt" name="Data/Audio/DemonHurt.wav"/>
      <Sound sound="Death" name="Data/Audio/NewDemonDeath.wav"/>
    </Sounds>
    <Inventory>
      <Weapon name="DemonMelee" />
    </Inventory>

    </ActorDefinition>
</Definitions>
```

