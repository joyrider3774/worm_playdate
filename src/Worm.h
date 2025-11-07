#ifndef WORM_H
#define WORM_H

#define ScaleFactor 1.5f
#define FPSScaleFactor 1.71f

#define ScreenWidth (int)(600 / ScaleFactor)
#define ScreenHeight (int)(360 / ScaleFactor)
#define FPS (int)(60 / FPSScaleFactor)

#define ObstacleWidth (int)(16 / ScaleFactor)
#define ObstacleHeight (int)(35 / ScaleFactor)
#define ObstacleSpaceFromTunnel (int)(10 / ScaleFactor)

#define PlayerWidthHeight 5
#define CollectibleWidth (int)(20 / ScaleFactor)
#define CollectibleHeight (int)(20 / ScaleFactor)
#define CollectibleSpaceFromTunnel (int)(30 / ScaleFactor)

#define StartTunnelSpeed 2
#define StartTunnelPlayableGap (int)(180 / ScaleFactor)
#define TunnelMinimumPlayableGap (int)(120 / ScaleFactor)
#define MaxTunnelSpeed 7
#define OffScreenTunnelSections 3

#define tunnelSectionWidth 10
#define tunnelSpacer 16
#define StartSpeedTarget (int)(50 * 8/tunnelSectionWidth)

#define ScreenBorderWidth (int)(7 / ScaleFactor)

#define Gravity (0.15f * FPSScaleFactor)

#define player_x (int)(250 / ScaleFactor)

#define MaxGameModes 5

// names for textures
#define TextureFullFont 0

// texture regions for full font texture
#define FirstRegionFullFont 0

#define InputDelay 16

#define maxSeed 101 //99 + 2 specials (0 = random(currenttimemilliseconds, but saves to 0), 1 = random(2, maxseed) 

extern PlaydateAPI* pd;
void setPDPtr(PlaydateAPI* p);
int mainLoop(void* ud);
void setupGame(void);
void terminateGame(void);
#endif