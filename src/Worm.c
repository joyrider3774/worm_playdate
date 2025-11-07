#include <string.h>
#include <pd_api.h>
#include <stdbool.h>
#include "Worm.h"
#include "SaveData.h"

LCDPattern kColorGrey = {
    // Bitmap
    0b10101010,
    0b01010101,
    0b10101010,
    0b01010101,
    0b10101010,
    0b01010101,
    0b10101010,
    0b01010101,

    // Mask
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
};

LCDPattern kColorGrey2 = {
    // Bitmap
    0b10001000,
    0b01000100,
    0b00100010,
    0b00010001,
    0b10001000,
    0b01000100,
    0b00100010,
    0b00010001,

    // Mask
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
};

LCDPattern kColorGrey3= {
    // Bitmap
    0b11101110,
    0b01110111,
    0b10111011,
    0b11011101,
    0b11101110,
    0b01110111,
    0b10111011,
    0b11011101,

    // Mask
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
};

typedef struct {
    int x;
    int y;
    int w;
    int h;
} SDL_FRect;

typedef struct {
    int x;
    int y;
} SDL_Point;


PlaydateAPI* pd;
PDButtons currButtons, prevButtons;
;
int score = 0, numTunnelSections = 0, numVisibleTunnelSections = 0, selSeed = 0, seed = 1, tunnelPlayableGap = StartTunnelPlayableGap, obstacleCount = 0, collectibleCount = 0, tunnelSpeed = StartTunnelSpeed;
int gameMode = 0, speedTarget = StartSpeedTarget, startDelay=0, MaxObstacles = 4, MaxCollectibles = 3;
float player_y = 250, playerSpeed = 0;
SDL_FRect tunnelParts[ScreenWidth*2 + OffScreenTunnelSections * 2]; // in case spacing is 1
SDL_Point playerTrail[ScreenWidth];
SDL_FRect obstacles[10];
SDL_FRect collectibles[10];
bool playing = false;


LCDFont* MonoFont;
bool showfps = false;

void setPDPtr(PlaydateAPI* p) {
    pd = p;
}

int randint(int min, int max)
{
    return (rand() % (max - min)) + min;
}

//intersection check instead of fully inside
int checkCollision(SDL_FRect* r1, SDL_FRect* r2)
{
    int x_left = (((r1->x) > (r2->x)) ? (r1->x) : (r2->x));
    int x_right = (((r1->x + r1->w) < (r2->x + r2->w)) ? (r1->x + r1->w) : (r2->x + r2->w));

    int y_top = (((r1->y) > (r2->y)) ? (r1->y) : (r2->y));
    int y_bottom = (((r1->y + r1->h) < (r2->y + r2->h)) ? (r1->y + r1->h) : (r2->y + r2->h));

    return ((x_right >= x_left) && (y_bottom >= y_top));
}

void drawTextColor(LCDFont* font, const char* text, size_t len, PDStringEncoding encoding, int x, int y, LCDColor color, bool inverted)
{
    int Lines = 1;
    size_t chars = 0;
    const char* p = text;
    while ((*p != '\0') && (chars < len))
    {
        if (*p == '\n')
            Lines++;
        p++;
        chars++;
    }

    int h = Lines * pd->graphics->getFontHeight(font);
    pd->graphics->setFont(font);
    int w = pd->graphics->getTextWidth(font, text, len, encoding, 0);
    //create new bitmap and fillrect with our color / pattern
    LCDBitmap *Bitmap = pd->graphics->newBitmap(w, h, kColorClear);
    if (inverted)
        pd->graphics->setDrawMode(kDrawModeInverted);
    pd->graphics->pushContext(Bitmap);
    pd->graphics->fillRect(0, 0, w, h, color);
    pd->graphics->popContext();

    //create mask with black background and draw text in white on the mask 
    LCDBitmap* bitmapmask = pd->graphics->newBitmap(w, h, kColorBlack);
    pd->graphics->pushContext(bitmapmask);
    pd->graphics->setDrawMode(kDrawModeFillWhite);
    pd->graphics->setFont(font);
    pd->graphics->drawText(text, len, encoding, 0, 0);
    pd->graphics->popContext();

    //set the mask to the bitmap with our pattern, this will make sure only the text
    //part (white in mask is drawn from the bitmap)
    pd->graphics->setBitmapMask(Bitmap, bitmapmask);
        
    //now draw the bitmap containing our text to the x & y position
    pd->graphics->drawBitmap(Bitmap, x, y, kBitmapUnflipped);
    pd->graphics->freeBitmap(Bitmap);
    pd->graphics->freeBitmap(bitmapmask);
}

void drawObstacles()
{
    //set cyan color
    for(int i = 0; i < obstacleCount; i++)
        //don't draw not used obstacles
        if((obstacles[i].x > -ObstacleWidth) && (obstacles[i].y > 0))
            pd->graphics->fillRect(obstacles[i].x, obstacles[i].y, obstacles[i].w, obstacles[i].h, (LCDColor)kColorGrey3);
}

void moveObstacles()
{
    //for each obstacle
    for (int i = 0; i < obstacleCount; i++)
        //move it at tunnelSpeed
        obstacles[i].x -= tunnelSpeed;
    
    //when have all obstacles on screen
    if (obstacleCount == MaxObstacles)
    {
        //for each obstacle
        for (int i = 0; i < obstacleCount; i++)
        {
            //if obstacle goes of screen to the left
            if(obstacles[i].x + obstacles[i].w < 0 )
            {
                //erase it from the array by moving all other obstalces one position down
                for (int j = 0; j < obstacleCount; j++)
                {
                    obstacles[j].x = obstacles[j+1].x;
                    obstacles[j].y = obstacles[j+1].y;
                }

                //and create a new obstacle at the right side of the screen
                obstacles[obstacleCount-1].x =  ScreenWidth;
                obstacles[obstacleCount-1].y =  tunnelParts[numVisibleTunnelSections*2].h + randint(ObstacleSpaceFromTunnel, tunnelPlayableGap - ObstacleHeight - 2*ObstacleSpaceFromTunnel);
                obstacles[obstacleCount-1].w = ObstacleWidth;
                obstacles[obstacleCount-1].h = ObstacleHeight;
            }
        }
    }

    //when we have no obstacles or the last added obstacle is smaller than the spacing between obstacles from right side of screen
    if ((obstacleCount == 0) || ((obstacleCount > 0) && (obstacleCount < MaxObstacles) && (obstacles[obstacleCount-1].x < ScreenWidth - (ScreenWidth / MaxObstacles))))
    {
        //add a new obstacles
        obstacles[obstacleCount].x =  ScreenWidth;
        obstacles[obstacleCount].y =  tunnelParts[numVisibleTunnelSections*2].h + randint(ObstacleSpaceFromTunnel, tunnelPlayableGap - ObstacleHeight - 2* ObstacleSpaceFromTunnel);
        obstacles[obstacleCount].w = ObstacleWidth;
        obstacles[obstacleCount].h = ObstacleHeight;
        obstacleCount++;
    }
}

void drawCollectibles()
{
    //set yellow color
    for(int i = 0; i < collectibleCount; i++)
        //don't draw not used collectible
        if((collectibles[i].x > 0) && (collectibles[i].y > 0))
            pd->graphics->fillRect(collectibles[i].x, collectibles[i].y, collectibles[i].w, collectibles[i].h, kColorWhite);
}

void moveCollectibles()
{
    //for each collectible
    for (int i = 0; i < collectibleCount; i++)
        //move it at tunnelSpeed
        collectibles[i].x -= tunnelSpeed;

    //when we have no collectible or the last added collectible is smaller than the spacing between collectible from right side of screen
    if ((collectibleCount == 0) || ((collectibleCount > 0) && (collectibleCount < MaxCollectibles) && (collectibles[collectibleCount-1].x < ScreenWidth - ((ScreenWidth - player_x) / MaxCollectibles))))
    {
        //add a new collectible
        collectibles[collectibleCount].x =  ScreenWidth;
        collectibles[collectibleCount].y =  tunnelParts[numVisibleTunnelSections*2].h + randint(CollectibleSpaceFromTunnel, tunnelPlayableGap - CollectibleHeight - 2 * CollectibleSpaceFromTunnel);
        collectibles[collectibleCount].w = CollectibleWidth;
        collectibles[collectibleCount].h = CollectibleHeight;
        collectibleCount++;
    }
}


void drawPlayer()
{
    //set yellow color
    pd->graphics->fillRect(player_x - 1, (int)(player_y) - 1, 3, 3, kColorWhite);
    for (int x = 0; x <=  player_x; x++)
    {
        //don't draw not used array pieces
        if ((playerTrail[x].y > 0) && (playerTrail[x].x > 0))
        {
            pd->graphics->fillRect(playerTrail[x].x - 1, playerTrail[x].y - 1, 3, 3, kColorWhite);
            if (x > 0)
                if ((playerTrail[x-1].y > 0) && (playerTrail[x-1].x > 0))
                    for (int y = 0; y < PlayerWidthHeight+1; y++)
                        pd->graphics->drawLine(playerTrail[x].x, playerTrail[x].y - 2 + y, (int)playerTrail[x - 1].x, (int)playerTrail[x - 1].y - 2 + y, 1, kColorWhite);
        }
    }
}

void movePlayer()
{
    if((currButtons & kButtonA) || (currButtons & kButtonB))
        playerSpeed += Gravity;
    else
        playerSpeed -= Gravity;

    player_y -= playerSpeed;
    
    //add position to player trail
    for (int x = 0; x <=  player_x; x++)
    {
        playerTrail[x].x = playerTrail[x+1].x-tunnelSpeed;
        playerTrail[x].y = playerTrail[x+1].y;
    }
    playerTrail[player_x].x = player_x;
    playerTrail[player_x].y = (int)player_y;

    SDL_FRect playerRect;
    playerRect.x = player_x -2;
    playerRect.y = (int)player_y -2;
    playerRect.w = PlayerWidthHeight;
    playerRect.h = PlayerWidthHeight;
    

    int playerTunnelSection = player_x / tunnelSectionWidth*2;
    //player is inside tunnel section
    for (int i = playerTunnelSection -4; i <= playerTunnelSection + 4; i++)
    {
        if (checkCollision(&playerRect, &tunnelParts[i]))
            playing = false;
    }

    //player is inside obstacle
    for (int i = 0; i < MaxObstacles; i++)
    {
        if (checkCollision(&playerRect, &obstacles[i]))
            playing = false;
    }

    
    for (int i = 0; i < MaxCollectibles; i++)
    {
        //player is inside collectible (added )
        if (checkCollision(&playerRect, &collectibles[i]))
        //debug
        //if (player_x > collectibles[i].x + collectibles[i].w)
        {
            //erase it from the array by moving all other obstalces one position down
            for (int j = 0; j < collectibleCount; j++)
            {
                collectibles[j].x = collectibles[j+1].x;
                collectibles[j].y = collectibles[j+1].y;
            }

            //and create a new obstacle at the right side of the screen
            if (collectibleCount > 0)
            {
                collectibles[collectibleCount - 1].x = ScreenWidth;
                collectibles[collectibleCount - 1].y = tunnelParts[numVisibleTunnelSections * 2].h + randint(CollectibleSpaceFromTunnel, tunnelPlayableGap - CollectibleHeight - 2 * CollectibleSpaceFromTunnel);
                collectibles[collectibleCount - 1].w = CollectibleWidth;
                collectibles[collectibleCount - 1].h = CollectibleHeight;
            }
        }

        //collectible is futher away than playerx (player missed to pick it up)
        if (player_x - (10/ScaleFactor) > collectibles[i].x + collectibles[i].w)
            playing = false;
    }

    //player is out of bounds
    if ((player_y < 0) || (player_y > ScreenHeight))
        playing = false;

    //debug
    //playing = true;
}

void createTunnel()
{
    //grab a height
    int top_height =  randint(0, tunnelPlayableGap);
    
    for(int i = 0; i <= numTunnelSections; i++)
    {
        //grab a height based on previous height with tunnelSpacer deviation of height
        top_height = randint(top_height - tunnelSpacer, top_height + tunnelSpacer);        
        
        //make sure it does not exceed our playable gap
        if (top_height < 0)
            top_height = 0;
        else
        {
            if (top_height > tunnelPlayableGap)
                top_height = tunnelPlayableGap;
        }
        
        //set player y position based on tunnel section where player is
        if((i * tunnelSectionWidth <= player_x) && ((i+1) * tunnelSectionWidth >= player_x))
            player_y = (float)(top_height + tunnelPlayableGap / 2);

        //top of tunnel
        tunnelParts[i*2].x = i * tunnelSectionWidth;
        tunnelParts[i*2].y = 0;
        tunnelParts[i*2].w = tunnelSectionWidth;
        tunnelParts[i*2].h = top_height;

        //bottom of tunnel
        tunnelParts[i*2+1].x = i * tunnelSectionWidth;
        tunnelParts[i*2+1].y = top_height + tunnelPlayableGap;
        tunnelParts[i*2+1].w = tunnelSectionWidth;
        tunnelParts[i*2+1].h = ScreenHeight - top_height - tunnelPlayableGap;
    }
}

void drawTunnel()
{
    //set green color
    for(int i = 0; i <= numTunnelSections * 2; i += 2)
    {
        pd->graphics->fillRect(tunnelParts[i].x, tunnelParts[i].y, tunnelParts[i].w, tunnelParts[i].h, (LCDColor)kColorGrey2);
        pd->graphics->fillRect(tunnelParts[i+1].x, tunnelParts[i+1].y, tunnelParts[i+1].w, tunnelParts[i+1].h, (LCDColor)kColorGrey2);
    }
}

void moveTunnel()
{
    //for every tunnel section
    for(int j = 0; j <= numTunnelSections; j++)
    {
        //move top & bottom tunnel part
        tunnelParts[j*2].x = tunnelParts[j*2].x - tunnelSpeed;
        tunnelParts[j*2+1].x = tunnelParts[j*2+1].x - tunnelSpeed;
    }
    
    bool increaseTunnelSpeed = false;

    for (int j = 0; j < numTunnelSections * 2; j++)
    {
        
        //if tunnel section are back on screen, break out of loop
        //tunnel sections are kept from left to right (lowest x)
        if (tunnelParts[j].x + tunnelSectionWidth >= 0)
            break;
        else
        //if left most tunnel sections is offscreen on the left
        {
            //erase that section from the arrray by moving all other section down in the array
            for (int i = 0; i <= numTunnelSections;i++)
            {
                tunnelParts[i*2].x = tunnelParts[i*2+2].x;
                tunnelParts[i*2].y = tunnelParts[i*2+2].y;
                tunnelParts[i*2].w = tunnelParts[i*2+2].w;
                tunnelParts[i*2].h = tunnelParts[i*2+2].h;
                tunnelParts[i*2+1].x = tunnelParts[i*2+3].x;
                tunnelParts[i*2+1].y = tunnelParts[i*2+3].y;
                tunnelParts[i*2+1].w = tunnelParts[i*2+3].w;
                tunnelParts[i*2+1].h = tunnelParts[i*2+3].h;
            }

            // create new piece at the end of the array
            int lastElement = numTunnelSections * 2;
            
            // place the new section exactly after the current rightmost
            int newX = tunnelParts[lastElement - 2].x + tunnelSectionWidth;

            // --- randomize top height (clamped as before) ---
            int top_height = randint(
                tunnelParts[lastElement - 2].h - tunnelSpacer,
                tunnelParts[lastElement - 2].h + tunnelSpacer
            );

            if (top_height < 0)
                top_height = 0;
            else
            {
                if (top_height > tunnelPlayableGap)
                    top_height = tunnelPlayableGap;
            }

            // --- assign new top & bottom tunnel parts ---
            tunnelParts[lastElement].x = newX;
            tunnelParts[lastElement].y = 0;
            tunnelParts[lastElement].w = tunnelSectionWidth;
            tunnelParts[lastElement].h = top_height;

            //bottom of tunnel
            tunnelParts[lastElement + 1].x = newX;
            tunnelParts[lastElement + 1].y = top_height + tunnelPlayableGap;
            tunnelParts[lastElement + 1].w = tunnelSectionWidth;
            tunnelParts[lastElement + 1].h = ScreenHeight - top_height - tunnelPlayableGap;

            //score increases with every section passed
            score += 1;
            if (seed < maxSeed)
            {
                if (score > save.highScores[gameMode * maxSeed + seed])
                    save.highScores[gameMode * maxSeed + seed] = score;
            }
            else if (score > save.highScores[gameMode * maxSeed])
                save.highScores[gameMode * maxSeed] = score;

            //make tunnel smaller
            if((gameMode == 0) || (gameMode == 3))
                if(tunnelPlayableGap > TunnelMinimumPlayableGap)
                    if(score % 4 == 0)
                        tunnelPlayableGap -= 1;
            
            //need to increase speed ?
            if((gameMode == 1) || (gameMode == 2) || (gameMode == 3))
                //if(tunnelSpeed < MaxTunnelSpeed)
                    if(score % (speedTarget) == 0)
                        increaseTunnelSpeed = true;
        }
    }

    if(increaseTunnelSpeed)
    {                        
        tunnelSpeed += 1;
        speedTarget *=2;
    }  
}

void startGame(int mode)
{
    if (selSeed == 0)
    {
        srand(pd->system->getCurrentTimeMilliseconds());
        seed = randint(2, maxSeed);
    }
    else
    {
        if (selSeed == 1)
        {
            srand(pd->system->getCurrentTimeMilliseconds());
            seed = randint(2, pd->system->getCurrentTimeMilliseconds());
        }
        else
            seed = selSeed;
    }
    srand(seed);
    playerSpeed = 0;
    tunnelPlayableGap = StartTunnelPlayableGap;
    score = 0;
    obstacleCount = 0;
    collectibleCount = 0;
    playing = true;
    //otherwise too difficult
    if (gameMode == 4)
        tunnelSpeed = 1;
    else
        tunnelSpeed = StartTunnelSpeed;
    speedTarget = StartSpeedTarget;
    gameMode = mode;
    startDelay = (int)(60/FPSScaleFactor);
    if (gameMode == 0)
        MaxObstacles = 4;
    if (gameMode == 2)
        MaxObstacles = 2;
    if (gameMode == 4)
        MaxCollectibles = 3;
    //set some defaults in the arrays
    for(int i = 0; i < ScreenWidth + OffScreenTunnelSections; i++)
    {
        if(i < ScreenWidth)
        {
            playerTrail[i].x = 0;
            playerTrail[i].y = 0;
        }
        tunnelParts[i*2].x = 0;
        tunnelParts[i*2+1].x = 0;
        tunnelParts[i*2].w = 0;
        tunnelParts[i*2+1].w = 0;
        tunnelParts[i*2].h = 0;
        tunnelParts[i*2+1].h = 0;
        tunnelParts[i*2].y = 0;
        tunnelParts[i*2+1].y = 0;
    }
    for(int i = 0 ; i < MaxObstacles; i++)
    {
        obstacles[i].x = ScreenWidth;
        obstacles[i].y = 0;
        obstacles[i].w = 0;
        obstacles[i].h = 0;
    }

    for(int i = 0 ; i < MaxCollectibles; i++)
    {
        collectibles[i].x = ScreenWidth;
        collectibles[i].y = 0;
        collectibles[i].w = 0;
        collectibles[i].h = 0;
    }
    createTunnel();
}

void drawBackGround()
{
    pd->graphics->clear(kColorBlack);
}

void drawScreenBorder()
{

    //A Darker green
    /*SDL_SetRenderDrawColor(Renderer, 0,66,0,255);
    SDL_FRect Rect;
    for (int i = 0; i < ScreenBorderWidth; i++)
    {
        Rect.x = i;
        Rect.y = i;
        Rect.w = ScreenWidth -2*i;
        Rect.h = ScreenHeight -2*i;
        SDL_RenderRect(Renderer, &Rect);
    }*/
}

void menuItemCallback(void* userdata)
{

    for (int i = 0; i < MaxGameModes * maxSeed; i++)
        save.highScores[i] = 0;
    SaveSavedData();
}

void setupGame()
{
    srand(pd->system->getCurrentTimeMilliseconds());
    const char* outErr = NULL;
    MonoFont = pd->graphics->loadFont("fonts/SpaceMono-Bold", &outErr);
    pd->graphics->setFont(MonoFont);
    pd->system->addMenuItem("Reset Scores", menuItemCallback, NULL);
    LoadSavedData();
    srand(pd->system->getCurrentTimeMilliseconds());
    //these never change and are used a lot in above functions
	numVisibleTunnelSections = (int)ceil(ScreenWidth / tunnelSectionWidth);
    numTunnelSections = numVisibleTunnelSections + OffScreenTunnelSections;
    createTunnel();
}

void terminateGame(void)
{

}

int mainLoop(void* ud)
{  
    int result = 1;

    prevButtons = currButtons;
    pd->system->getButtonState(&currButtons, NULL, NULL);

    char* Text;
    char* Text2;
    drawBackGround();
    drawTunnel();
    if((gameMode == 0) || (gameMode == 2))
        drawObstacles();
    if(gameMode == 4)
        drawCollectibles();
    drawPlayer();
    drawScreenBorder();
    if(playing)
    {
        if(startDelay == 0)
        {
            moveTunnel();
            if((gameMode == 0) || (gameMode == 2))
                moveObstacles();
            if(gameMode == 4)
                moveCollectibles();
            movePlayer();
            if (!playing)
            {
                SaveSavedData();
                pd->system->addMenuItem("Reset Scores", menuItemCallback, NULL);
            }
        }
        else
        {
            int fh = pd->graphics->getFontHeight(MonoFont);
            startDelay--;
            if(startDelay > (20/FPSScaleFactor))
            {
                pd->system->formatString(&Text2, "Playing GAME A");
                Text2[13] = 'A' + gameMode;
                int fw = pd->graphics->getTextWidth(MonoFont, Text2, strlen(Text2), kASCIIEncoding, 0);
                drawTextColor(MonoFont,Text2, strlen(Text2), kASCIIEncoding, (ScreenWidth - fw) >> 1, (ScreenHeight >> 1) - fh, kColorWhite, false);
                pd->system->realloc(Text2, 0);

                pd->system->formatString(&Text2, "READY");
                fw = pd->graphics->getTextWidth(MonoFont, Text2, strlen(Text2), kASCIIEncoding, 0);
                drawTextColor(MonoFont, Text2, strlen(Text2), kASCIIEncoding, (ScreenWidth - fw) >> 1, (ScreenHeight >> 1), kColorWhite, false);
                pd->system->realloc(Text2, 0);
                
            }
            else
            {
                if (startDelay > 1)
                {
                    int fw = pd->graphics->getTextWidth(MonoFont, "GO!", strlen("GO!"), kASCIIEncoding, 0);
                    drawTextColor(MonoFont, "GO!", strlen("GO!"), kASCIIEncoding, (ScreenWidth - fw) >> 1, (ScreenHeight - fh) >> 1, kColorWhite, false);
                }
            }
        
        }
    }
    else
    {
        char* Text;        
        pd->system->formatString(&Text, "WORM");
        int fw = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);
        int fh = pd->graphics->getFontHeight(MonoFont);
        drawTextColor(MonoFont, Text, strlen(Text), kASCIIEncoding, (ScreenWidth - fw) >> 1, ScreenBorderWidth, kColorWhite, false);
        pd->system->realloc(Text, 0);
        
        pd->system->formatString(&Text, "Press A or B To Play GAME A");
        Text[26] = 'A' + gameMode;
        fw = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);
        drawTextColor(MonoFont, Text, strlen(Text), kASCIIEncoding, (ScreenWidth - fw) >> 1, ScreenBorderWidth + 1 * fh + 10, kColorWhite, false);
        pd->system->realloc(Text, 0);
        
        pd->system->formatString(&Text, "Press Direction To Change Mode");
        fw = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);
        drawTextColor(MonoFont, Text, strlen(Text), kASCIIEncoding, (ScreenWidth - fw) >> 1, ScreenBorderWidth + 2 * fh + 10, kColorWhite, false);
        pd->system->realloc(Text, 0);

        pd->system->formatString(&Text, "Pressing A or B Repeadetly");
        fw = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);
        drawTextColor(MonoFont, Text, strlen(Text), kASCIIEncoding, (ScreenWidth - fw) >> 1, ScreenBorderWidth + 3 * fh + 20, kColorWhite, false);
        pd->system->realloc(Text, 0);

        pd->system->formatString(&Text, "will keep the worm alive");
        fw = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);
        drawTextColor(MonoFont, Text, strlen(Text), kASCIIEncoding, (ScreenWidth - fw) >> 1, ScreenBorderWidth + 4 * fh + 20, kColorWhite, false);
        pd->system->realloc(Text, 0);

        if((!(prevButtons & kButtonA) && (currButtons & kButtonA)) ||
            (!(prevButtons & kButtonB) && (currButtons & kButtonB)))
        {
            pd->system->removeAllMenuItems();
            startGame(gameMode);
        }

        if ((!(prevButtons & kButtonLeft) && (currButtons & kButtonLeft)))
        {
            gameMode--;
            if (gameMode < 0)
                gameMode = MaxGameModes -1;
        }

        if ((!(prevButtons & kButtonRight) && (currButtons & kButtonRight)))
        {
            gameMode++;
            if (gameMode > MaxGameModes -1)
                gameMode = 0;
        }

        if ((!(prevButtons & kButtonDown) && (currButtons & kButtonDown)))
        {
            selSeed -= 1;
            if(selSeed < 0)
                selSeed = maxSeed-1;
        }

        if ((!(prevButtons & kButtonUp) && (currButtons & kButtonUp)))
        {
            selSeed += 1;
            if(selSeed > maxSeed-1)
                selSeed = 0;
        }
    }
	if (selSeed == 0)
        pd->system->formatString(&Text, "LVL:%d Rnd1", seed-1);
    else
    {
        if (selSeed == 1)
            pd->system->formatString(&Text, "LVL:%d Rnd2", seed-1);
        else
            pd->system->formatString(&Text, "LVL:%d", selSeed-1);
    }

    int w, h;
    h = pd->graphics->getFontHeight(MonoFont);
    w = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);    
    drawTextColor(MonoFont,Text, strlen(Text), kASCIIEncoding, ScreenBorderWidth + 1, ScreenHeight - h, kColorWhite, false);
    pd->system->realloc(Text, 0);

    if (selSeed <= 1)
    {
        if (seed >= maxSeed)
            pd->system->formatString(&Text, "S:%d h:%d", score, save.highScores[gameMode * maxSeed]);
        else
            pd->system->formatString(&Text, "S:%d h:%d", score, save.highScores[gameMode * maxSeed + seed]);
    }
    else if (selSeed > 1)
        pd->system->formatString(&Text, "S:%d h:%d", score, save.highScores[gameMode * maxSeed + selSeed]);

    w = pd->graphics->getTextWidth(MonoFont, Text, strlen(Text), kASCIIEncoding, 0);
    drawTextColor(MonoFont,Text, strlen(Text), kASCIIEncoding, ScreenWidth - 2 - ScreenBorderWidth - w, ScreenHeight - h, kColorWhite, false);
    pd->system->realloc(Text, 0);

    if (showfps)
        pd->system->drawFPS(0, 0);
    return result;
}
