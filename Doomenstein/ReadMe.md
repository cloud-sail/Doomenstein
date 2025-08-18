# Doomenstein Gold

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

## Intro
Become the Doom Slayer, relentlessly hunting demons that hide in the shadows. 
Demons run away fast and hide behind the wall, but
- you can use a long range pistol to stagger them (weapon 1)
- you can use a brutal melee attack for instant glory kills (weapon 2)
- you can use radar to locate them (weapon 3)

## Gold Features
### Weapon
- Radar
	- show the position in the screen coordinates with distance and health for 5 seconds
- Guitar
	- When equipping this weapon, the music will change to a heavy metal music, and when you are in attack animation, the volume will goes up.
	- When moving and swing the guitar, player will have a dash. and the damage collsion is a 2D capsule (not a sector), because the player is moving when attacking
	- Guitar will instantly kill the demon, and play a glory kill animation under third person camera.

### AI
- AI will try best to avoid the sight line of the player
	- First According to the location of the players and the sight range to do a ray cast to each tile center. Generate a heatmap(exposed/unexposed)
	- Then increase the heat from unexposed tiles, which will generate a heat map for the AI to go downhill.
	- Just going to the tile of which the center is not visible is not enough, because some parts of the demon will be attacked. The heatmap need to decrease 1 heat.
	- I generate a flow field from the heat map. And using bilinear interpolation to generate moving direction at any position.
	- AI then follow the direction of current location

- AI has three states: Idle, Flee, Stagger
	- If they are hit or see the player, they will enter flee state
	- If they receive certain number of damage, they will enter stagger state(not move) for 3 seconds and shine blue.
	- If the stagger time is over, they will enter flee state again
	- If player die, back to idle.

### Game Mechanics
- Lock on the enemy
	- Some people may be poor at FPS game using mouses or gamepads, I made a lock on system to help the player to lock the enemy in the center of the screen.
	- First calculate the screen position of each enemy and cull out demons not inside the screen, and do fast voxel raycast to check visibility.
	- Then select the demon which is closest to the center of the screen.
	- If the player presses the lockon button, the camera will look at that enemy.

- When dashing, there is a speed line around the screen and will gradually fade out.
- If you do damage on a enemy, there is a sign which shows you hit a enemy around the reticle. It will also fade out.

- Weapn shaking: according to the current speed, the weapon sprite will move like a pendulum to make the player feels they are running
- Weapon switch animation: when switching the weapon, the weapon will move up from the bottom of the screen.

### Technical Features
- Skybox


## Known Bugs
- In multiplayer mode, the logic for switch between game music and special weapon music will have problems. Because both players will affect the music switching and volume changing.
	- Already have some ideas but have not time to fix
	- (using reference count so that the special music pauses only when all players request a pause and the volume deactivates only when all players request deactivation.)


