#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "object.h"



object_t *g_objects;
sector_t g_sectors[NUM_SECTORS_WID][NUM_SECTORS_HIG];

// counters & performance
unsigned g_timestamp;
unsigned g_colSearchCalls, g_collCalls;

// world options
char g_worldGrav;
float g_worldGravDir;
char g_gravStar;
char g_elasticCollisions;
border_bounce_t g_borderType;

// local global for numbering objects
unsigned id_counter;



void initObjects(){
  printf("NUM_SECTORS_WID: %d\n", NUM_SECTORS_WID);
  printf("NUM_SECTORS_HIG: %d\n", NUM_SECTORS_HIG);
  printf("WORLD_WID: %d\n", WORLD_WID);
  printf("WORLD_HIG: %d\n", WORLD_HIG);
  printf("SECTOR_SIZE: %d\n", SECTOR_SIZE);

  g_objects = NULL;
  for(int i=0;i<NUM_SECTORS_WID;++i)
    for(int j=0;j<NUM_SECTORS_HIG;++j)
      g_sectors[i][j].timestamp = 0;
  
  g_colSearchCalls = 0;
  g_collCalls = 0;
  
  g_worldGrav = 0;
  g_worldGravDir = 24 * (15.0 * (float)rand() / (RAND_MAX+1.0)) * 2 * M_PI / 360;
  g_gravStar = 0;
  g_elasticCollisions = 1;
  g_borderType = BORDER_BOUNCE;
  
  srand(0);

  addSomeObjects(NUM_OBJ);
}



void addSomeObjects(int num){
  for(int i=0;i<num;++i){
    object_t *oPtr = malloc(sizeof(object_t));
    initObject_t(oPtr);
    sortObject_t(oPtr);
  }
}



void initObject_t(object_t *oPtr){
  oPtr->id = ++id_counter;
  oPtr->alive = 1;
  
  oPtr->x = WORLD_MIN_X + WORLD_WID * (float)rand() / (RAND_MAX+1.0);
  oPtr->y = WORLD_MIN_Y + WORLD_HIG * (float)rand() / (RAND_MAX+1.0);
  oPtr->dx = -0.125 + 0.25 * (float)rand() / (RAND_MAX+1.0);
  oPtr->dy = -0.125 + 0.25 * (float)rand() / (RAND_MAX+1.0);
  oPtr->radius = 1.0 + 5.0 * (float)rand() / (RAND_MAX+1.0);
  
  int choose = 1 + (int)(8.0 * (float)rand() / (RAND_MAX+1.0));
  if( choose == 1 )
    oPtr->radius *= 2;
  else if( choose > 4 )
    oPtr->radius /= 2;
  
  // shortcut for starting them at large sizes for testing
  oPtr->radius *= BASE_SIZE;
  
  oPtr->heat = 0.0;

  oPtr->prev = NULL;
  oPtr->next = NULL;
}



// put the given object_t into the global list
// the largest object needs to be first for the in-sector tree model to work right
void sortObject_t(object_t *oPtr){
  object_t *leadPtr = g_objects;
  if( leadPtr == NULL || leadPtr->radius < oPtr->radius ){
    g_objects = oPtr;
    oPtr->next = leadPtr;
  }else{
    object_t *followPtr = leadPtr;
    leadPtr = leadPtr->next;
    
    while( leadPtr && leadPtr->radius > oPtr->radius ){
      followPtr = leadPtr;
      leadPtr = leadPtr->next;
    }
    
    followPtr->next = oPtr;
    oPtr->next = leadPtr;
    oPtr->prev = followPtr;
  }
  
  if( oPtr->next )
    oPtr->next->prev = oPtr;
}



// increase the radii of all objects
void radiusChange(float mod){
  object_t *oPtr = g_objects;
  while( oPtr ){
    oPtr->radius *= mod;
    oPtr = oPtr->next;
  }
}



// increase the rate of movement of all objects
void speedChange(float mod){
  object_t *oPtr = g_objects;
  while( oPtr ){
    oPtr->dx *= mod;
    oPtr->dy *= mod;
    oPtr = oPtr->next;
  }
}



// use the first element in the list to cancel out the movement of all of them
void cancelMomentum(){
  list_stats_t stats;
  listStats(&stats);
  
  if( g_objects ){
    g_objects->dx -= stats.inertiaX / (g_objects->radius*g_objects->radius);
    g_objects->dy -= stats.inertiaY / (g_objects->radius*g_objects->radius);
  }
}



void setStar(){
  if( !g_objects ) return;
  
  if( g_gravStar )
    g_objects->radius *= 3;
  else
    g_objects->radius /= 3;
}



void updateObject_t(object_t *oPtr){
  
  if( g_borderType == BORDER_BOUNCE ){
    // bounce off the walls
    
    if( oPtr->x - oPtr->radius < WORLD_MIN_X && oPtr->dx < 0.0 )
      oPtr->dx *= -1;
    else if( oPtr->x + oPtr->radius > WORLD_MAX_X && oPtr->dx > 0.0 )
      oPtr->dx *= -1;
    if( oPtr->y - oPtr->radius < WORLD_MIN_Y && oPtr->dy < 0.0 )
      oPtr->dy *= -1;
    else if( oPtr->y + oPtr->radius > WORLD_MAX_Y && oPtr->dy > 0.0 )
      oPtr->dy *= -1;
      
  }else if( g_borderType == BORDER_WRAP ){
    // wrap around the sides rather than bounce
    
    if( oPtr->x + oPtr->radius < WORLD_MIN_X && oPtr->dx < 0.0 )
      oPtr->x += WORLD_WID + 2*oPtr->radius;
    else if( oPtr->x - oPtr->radius > WORLD_MAX_X && oPtr->dx > 0.0 )
      oPtr->x -= WORLD_WID + 2*oPtr->radius;
    if( oPtr->y + oPtr->radius < WORLD_MIN_Y && oPtr->dy < 0.0 )
      oPtr->y += WORLD_HIG + 2*oPtr->radius;
    else if( oPtr->y - oPtr->radius > WORLD_MAX_Y && oPtr->dy > 0.0 )
      oPtr->y -= WORLD_HIG + 2*oPtr->radius;
  }
  
  // movement
  oPtr->x += oPtr->dx;
  oPtr->y += oPtr->dy;

  // cooldown
  if( oPtr->heat > 0.0 ){
    oPtr->heat *= (1.0 - 0.1 / (oPtr->radius*oPtr->radius));
    if( oPtr->heat < 0.01 ) oPtr->heat = 0.0;
  }
  
  // world gravity
  if( g_worldGrav ){
    oPtr->dx -= WORLD_GRAV_ACC * cos(g_worldGravDir);
    oPtr->dy -= WORLD_GRAV_ACC * sin(g_worldGravDir);
  }
  
  // star gravity
  if( g_gravStar && g_objects != oPtr ){
    float xDist = g_objects->x - oPtr->x;
    float yDist = g_objects->y - oPtr->y;
    float dist = xDist * xDist + yDist * yDist;
    
    if( sqrt(dist) > g_objects->radius + oPtr->radius ){
      float bigMass = g_objects->radius * g_objects->radius;
      float lilMass = oPtr->radius * oPtr->radius;
      
      float accelStar = STAR_GRAVITY * lilMass / dist;
      float accelObj = STAR_GRAVITY * bigMass / dist;
      
      float angle = atan( yDist / xDist );
      
      if(xDist < 0){
        oPtr->dx -= accelObj * cos(angle);
        oPtr->dy -= accelObj * sin(angle);
        g_objects->dx += accelStar * cos(angle);
        g_objects->dy += accelStar * sin(angle);
      }else{
        oPtr->dx += accelObj * cos(angle);
        oPtr->dy += accelObj * sin(angle);
        g_objects->dx -= accelStar * cos(angle);
        g_objects->dy -= accelStar * sin(angle);
      }
    }
  }
}



void listStats(list_stats_t *stats){
  if( !stats ) return;
  
  stats->timestamp = g_timestamp;
  stats->numObj = 0;
  stats->kineticEng = 0.0;
  stats->inertiaX = 0.0;
  stats->inertiaX = 0.0;
  stats->centerMassX = 0.0;
  stats->centerMassY = 0.0;
  stats->mass = 0.0;
  
  object_t *oPtr = g_objects;
  while( oPtr ){
    float mass = oPtr->radius*oPtr->radius;
    
    stats->numObj += 1;
    stats->kineticEng += mass*(oPtr->dx*oPtr->dx + oPtr->dy*oPtr->dy);
    stats->inertiaX += mass*oPtr->dx;
    stats->inertiaY += mass*oPtr->dy;
    stats->centerMassX = (oPtr->x * mass + stats->centerMassX * stats->mass) / (mass + stats->mass);
    stats->centerMassY = (oPtr->y * mass + stats->centerMassY * stats->mass) / (mass + stats->mass);
    stats->mass += mass;
    
    oPtr = oPtr->next;
  }
}



// allow some level of result checking
// note that comparison with other collision detection algorithms is only
//   possible if both algos resolve simultaneous collisions in the same order
unsigned statSum(){
  unsigned result = 0;
  
  object_t *oPtr = g_objects;
  while( oPtr ){
    #ifdef OBJECT_DEBUG
    printf("#%d: (x , y) = (%.1f , %.1f)\n",oPtr->id,oPtr->x,oPtr->y);
    #endif
    
    result += (int)oPtr->x;
    result += (int)oPtr->y;
    oPtr = oPtr->next;
  }
  
  return result;
}



void collide(object_t *obj1, object_t *obj2){
  ++g_collCalls;
  
  if( g_gravStar && obj1 == g_objects ){
    obj2->alive = 0;
    obj1->dx += (obj2->dx*obj2->radius*obj2->radius) / (obj1->radius*obj1->radius);
    obj1->dy += (obj2->dy*obj2->radius*obj2->radius) / (obj1->radius*obj1->radius);
    obj1->radius = sqrt(obj1->radius*obj1->radius + obj2->radius+obj2->radius);
    fprintf(stderr,"Object %u should be destroyed.\n",obj2->id);
    return;
  }
  
  float dx, dy, x, y;
  float dxy, v, v1, v2, nullV;
  float incomingAngle, centersAngle, nullAngle;
  float mass1 = obj1->radius * obj1->radius;
  float mass2 = obj2->radius * obj2->radius;
  
  // make things stationary around the 2nd object to ease calculations
  dx = obj1->dx - obj2->dx;
  dy = obj1->dy - obj2->dy;
  
  // make 2nd object's coordinates 0,0 to ease calculations
  x = obj1->x - obj2->x;
  y = obj1->y - obj2->y;
  
  // not moving relative to each other, therefore cannot be colliding
  if(dx == 0 && dy == 0 )
    return;
  
  // forget the special cases (this effects only calc of angles)
  if(x == 0) x = .0000001;
  if(dx == 0) dx = .0000001;
  
  // speed of closure
  dxy = sqrt(dx * dx + dy * dy);      // closing velocity
  
  // angle that the 1st object is "approaching" from
  incomingAngle = atan(dy/dx);
  if(dx < 0) incomingAngle += M_PI;
  if(incomingAngle < 0) incomingAngle += 2 * M_PI;
  // should result in a 0-360 deg range of angles
  // 270 to 360: lower right
  // 180 to 270: lower left
  // 90 to 180:  upper left
  // 0 to 90:    upper right
  
  // angle that the object centers are at
  centersAngle = atan(y/x);
  if(x < 0) centersAngle += M_PI;
  if(centersAngle < 0) centersAngle += 2 * M_PI;
  // should result in a 0-360 deg range of angles
  // 270 to 360: lower right
  // 180 to 270: lower left
  // 90 to 180:  upper left
  // 0 to 90:    upper right
  
  // if the angles are within 90 degrees, there can be no colission
  // 180 degree separation means straight-on collision
  if( fabsf(incomingAngle - centersAngle) < M_PI / 2 ) return;
  if( fabsf(incomingAngle - centersAngle) > 3 * M_PI / 2 ) return;
    
  nullAngle = incomingAngle - centersAngle;
  nullV = dxy * sin(nullAngle);    // non-collision v (relative of course)
  v = dxy * cos(nullAngle);        // collision v, head-on

  if( !g_elasticCollisions ){
    v *= 0.98;
    obj1->heat += fabsf(v * (mass1 + mass2) / mass1);
    obj2->heat += fabsf(v * (mass1 + mass2) / mass2);  // hmm
    //printf("heat %f & %f",obj1->heat,obj2->heat);
  }
  
  // one-dimentional elastic collision
  // (solutions to cons of momentum and cons of energy)
  v1 = v * ((mass1 - mass2) / (mass1 + mass2));
  v2 = v * ((2 * mass1) / (mass1 + mass2));
  
  obj1->dx = obj2->dx + cos(centersAngle) * v1 - sin(centersAngle) * nullV;
  obj1->dy = obj2->dy + sin(centersAngle) * v1 + cos(centersAngle) * nullV;
  obj2->dx = obj2->dx + cos(centersAngle) * v2;
  obj2->dy = obj2->dy + sin(centersAngle) * v2;
}



static inline float dist(object_t *obj1, object_t *obj2){
  float x = obj1->x - obj2->x;
  float y = obj1->y - obj2->y;
  return sqrt(x*x + y*y);
}



// recursive function to search tree for collisions
void colSearchObject_t(object_t *tree, object_t *oPtr){
  ++g_colSearchCalls;
  
  if( tree->radius + oPtr->radius > fabsf(tree->x - oPtr->x) ){
    if( tree->radius + oPtr->radius > fabsf(tree->y - oPtr->y) )
      if( tree->radius + oPtr->radius > dist(tree,oPtr) )
        collide(tree,oPtr);
    
    if( tree->less )
      colSearchObject_t(tree->less,oPtr);
    if( tree->more )
      colSearchObject_t(tree->more,oPtr);
  }else{
    if( (tree->x - oPtr->x) < 0 && tree->less )
      colSearchObject_t(tree->less,oPtr);
    else if( tree->more )
      colSearchObject_t(tree->more,oPtr);
  }
}



// store the object into the sector it falls in
void entreeObject_t(object_t *oPtr, sector_t *sPtr){
  oPtr->less = NULL;
  oPtr->more = NULL;
  
  if( sPtr->timestamp != g_timestamp ){
    // the sector has stale data in it (overwrite it)
    sPtr->tree = oPtr;
    // mark the sector as having something in it
    sPtr->timestamp = g_timestamp; 
  }else{
    object_t *nodePtr = sPtr->tree;
    while( 1 ){
      if( nodePtr->x > oPtr->x ){
        if( nodePtr->more ){
          nodePtr = nodePtr->more;
        }else{
          nodePtr->more = oPtr;
          break;
        }
      }else{
        if( nodePtr->less ){
          nodePtr = nodePtr->less;
        }else{
          nodePtr->less = oPtr;
          break;
        }
      }
    }
  }
}



void engridObject_t(object_t *oPtr){
  if(!oPtr->alive) return;

  // convert x coordinate into a sector index
  int xindex = (int)oPtr->x / SECTOR_SIZE;
  if( xindex < UNDERFLOW_SECTOR ) xindex = UNDERFLOW_SECTOR;
  if( xindex > OVERFLOW_SECTOR_X ) xindex = OVERFLOW_SECTOR_X;
  
  // convert y coordinate into a sector index
  int yindex = (int)oPtr->y / SECTOR_SIZE;
  if( yindex < UNDERFLOW_SECTOR ) yindex = UNDERFLOW_SECTOR;
  if( yindex > OVERFLOW_SECTOR_Y ) yindex = OVERFLOW_SECTOR_Y;
  
  // condensed method of safely trying +/- sectors for both x/y coords
  for(int i=-1;i<2;++i)
    if( xindex+i >= UNDERFLOW_SECTOR && xindex+i <= OVERFLOW_SECTOR_X )
      for(int j=-1;j<2;++j)
        if( yindex+j >= UNDERFLOW_SECTOR && yindex+j <= OVERFLOW_SECTOR_Y )
          if( g_sectors[xindex+i][yindex+j].timestamp == g_timestamp )
            colSearchObject_t(g_sectors[xindex+i][yindex+j].tree,oPtr);
  
  // store the object into the sector it falls in
  entreeObject_t(oPtr,&g_sectors[xindex][yindex]);
}
