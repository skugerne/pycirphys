#ifndef OBJECT_H
#define OBJECT_H



typedef struct object_t {
  unsigned id;
  char alive;            // death happens when hitting the star, maybe more later
  
  float x,y;
  float dx,dy;
  float radius;
  float heat;            // increased by collisions, decreased over time
  
  struct object_t *less;        // to node at lower coordinate
  struct object_t *more;        // to node at higher coordinate
  
  struct object_t *prev;        // for linking into a list
  struct object_t *next;        // for linking into a list
} object_t;

typedef struct sector_t {
  unsigned timestamp;
  object_t *tree;
} sector_t;

typedef struct list_stats_t {
  unsigned timestamp;
  
  unsigned numObj;
  
  float kineticEng;
  float inertiaX;
  float inertiaY;
  float centerMassX;
  float centerMassY;
  float mass;
} list_stats_t;

typedef enum border_bounce_t {
  BORDER_BOUNCE,
  BORDER_WRAP,
  BORDER_OPEN
} border_bounce_t;


#define OBJECT_DEBUG

//#define LONG_WAY
#define NUM_OBJ 1024
#define BASE_SIZE 1
#define OBJ_MIN_SIDES 3
#define WORLD_GRAV_ACC 0.00015

#define STAR_GLOW_1 2.0
#define STAR_GLOW_2 5.0
#define STAR_POINTS 64
#define STAR_SIDES 16
#define STAR_GRAVITY .005

#define SECTOR_SIZE 16       // power of 2 will perform slightly better
#define NUM_SECTORS_WID 63
#define NUM_SECTORS_HIG 47

#define WORLD_MIN_X 0
#define WORLD_MAX_X (SECTOR_SIZE*NUM_SECTORS_WID)
#define WORLD_WID (WORLD_MAX_X-WORLD_MIN_X)
#define WORLD_MIN_Y 0
#define WORLD_MAX_Y (SECTOR_SIZE*NUM_SECTORS_HIG)
#define WORLD_HIG (WORLD_MAX_Y-WORLD_MIN_Y)
#define UNDERFLOW_SECTOR 0
#define OVERFLOW_SECTOR_X (NUM_SECTORS_WID-1)
#define OVERFLOW_SECTOR_Y (NUM_SECTORS_HIG-1)



extern object_t *g_objects;
extern sector_t g_sectors[NUM_SECTORS_WID][NUM_SECTORS_HIG];

// counters & performance
extern unsigned g_timestamp;
extern unsigned g_colSearchCalls, g_collCalls;

// world options
extern char g_worldGrav;
extern float g_worldGravDir;
extern char g_gravStar;
extern char g_elasticCollisions;
extern border_bounce_t g_borderType;



void initObjects();
void addSomeObjects(int);
void initObject_t(object_t*);
void sortObject_t(object_t*);
void radiusChange(float);
void speedChange(float);
void cancelMomentum();
void setStar();
void updateObject_t(object_t*);
void listStats(list_stats_t*);
unsigned statSum();
void engridObject_t(object_t*);



#endif
