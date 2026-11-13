/*
    NEON VANGUARD: RIFT PROTOCOL
    A self-contained 3D arena survival game for Code::Blocks + FreeGLUT.
    Controls:
      WASD move | Mouse aim | Left click fire | R reload | SPACE dash
      1/2/3 weapon | SHIFT sprint | P/ESC pause | ENTER start/restart
*/
#include <GL/freeglut.h>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <ctime>

const float PI = 3.1415926535f;
const int W = 1280, H = 800;
float clampf(float v,float a,float b){return v<a?a:(v>b?b:v);}
float rnd(float a,float b){return a+(b-a)*(rand()/(float)RAND_MAX);}
struct V3{
    float x,y,z;
    V3(float X=0,float Y=0,float Z=0):x(X),y(Y),z(Z){}
    V3 operator+(V3 b)const{return V3(x+b.x,y+b.y,z+b.z);}
    V3 operator-(V3 b)const{return V3(x-b.x,y-b.y,z-b.z);}
    V3 operator*(float s)const{return V3(x*s,y*s,z*s);}
};
float len(V3 a){return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);}
V3 norm(V3 a){float l=len(a);return l>0.0001f?a*(1.0f/l):V3();}
float dist(V3 a,V3 b){return len(a-b);}
float dot2(V3 a,V3 b){return a.x*b.x+a.z*b.z;}

enum GameState{TITLE,PLAYING,PAUSED,DEAD,VICTORY};
enum Weapon{PULSE,SHOTGUN,RAIL};
struct Enemy{V3 p; float hp,speed,radius,attack,phase; int type; bool alive;};
struct Bullet{V3 p,v; float life,damage; bool hostile;};
struct Pickup{V3 p; int type; float spin; bool alive;};
struct Particle{V3 p,v; float life,maxLife,size; float r,g,b;};
struct Obstacle{V3 p; float sx,sy,sz;};

GameState state=TITLE;
std::vector<Enemy> enemies;
std::vector<Bullet> bullets;
std::vector<Pickup> pickups;
std::vector<Particle> particles;
std::vector<Obstacle> obstacles;
V3 player(0,0,0), cam(0,7,12);
float yaw=0, pitch=0.22f, health=100, shield=60, energy=100;
float mouseX=W/2,mouseY=H/2;
bool keys[256]={0}, specialKeys[256]={0}, mouseDown=false, sprinting=false;
int wave=0,kills=0,score=0,ammo=30,magSize=30,reserveAmmo=180;
Weapon weapon=PULSE;
float fireCooldown=0,reloadTimer=0,dashCooldown=0,dashTime=0,invuln=0;
float waveDelay=0,spawnTimer=0,gameTime=0,shake=0;
float muzzle=0,hitFlash=0;
std::string notice="RIFT SIGNATURE DETECTED";
float noticeTime=3.0f;

void text(float x,float y,const std::string&s,void*font=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(x,y); for(char c:s) glutBitmapCharacter(font,c);
}
void textCenter(float x,float y,const std::string&s,void*f=GLUT_BITMAP_HELVETICA_18){
    float width=0; for(char c:s) width+=glutBitmapWidth(f,c);
    text(x-width/2,y,s,f);
}
void quad(float x,float y,float w,float h,float r,float g,float b,float a){
    glColor4f(r,g,b,a); glBegin(GL_QUADS); glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
void bar(float x,float y,float w,float h,float val,float maxv,float r,float g,float b){
    quad(x,y,w,h,0.025f,0.04f,0.08f,0.88f);
    quad(x+2,y+2,(w-4)*clampf(val/maxv,0,1),h-4,r,g,b,0.95f);
}
void addParticle(V3 p,V3 v,float life,float size,float r,float g,float b){
    Particle q; q.p=p;q.v=v;q.life=q.maxLife=life;q.size=size;q.r=r;q.g=g;q.b=b;particles.push_back(q);
}
void burst(V3 p,int n,float r,float g,float b){
    for(int i=0;i<n;i++){float a=rnd(0,2*PI),s=rnd(1,7);addParticle(p,V3(cosf(a)*s,rnd(1,7),sinf(a)*s),rnd(.25f,.8f),rnd(.04f,.13f),r,g,b);}
}
bool blocked(V3 p){
    for(size_t i=0;i<obstacles.size();i++){
        Obstacle o=obstacles[i];
        if(fabsf(p.x-o.p.x)<o.sx*.5f+.45f && fabsf(p.z-o.p.z)<o.sz*.5f+.45f) return true;
    } return false;
}
void buildArena(){
    obstacles.clear();
    for(int i=0;i<18;i++){
        float a=i*2*PI/18,rad=rnd(12,19);
        Obstacle o; o.p=V3(cosf(a)*rad,0,sinf(a)*rad);o.sx=rnd(1.2f,2.8f);o.sy=rnd(1.8f,4.5f);o.sz=rnd(1.2f,2.8f);obstacles.push_back(o);
    }
    for(int i=0;i<8;i++){Obstacle o;o.p=V3(rnd(-8,8),0,rnd(-8,8));o.sx=rnd(1,2);o.sy=rnd(1.2f,3);o.sz=rnd(1,2);if(len(o.p)>4)obstacles.push_back(o);}
}
void resetGame(){
    enemies.clear();bullets.clear();pickups.clear();particles.clear();
    player=V3(0,0,0);yaw=0;pitch=.22f;health=100;shield=60;energy=100;
    wave=0;kills=0;score=0;ammo=30;reserveAmmo=180;weapon=PULSE;
    fireCooldown=reloadTimer=dashCooldown=dashTime=invuln=gameTime=0;
    waveDelay=1;spawnTimer=0;shake=0;buildArena();state=PLAYING;
    notice="RIFT BREACH: SURVIVE";noticeTime=3;
}
void spawnWave(){
    wave++;
    int count=3+wave*2;
    for(int i=0;i<count;i++){
        float a=rnd(0,2*PI),r=rnd(23,29);
        Enemy e;e.p=V3(cosf(a)*r,0,sinf(a)*r);e.type=(wave>2 && rand()%5==0)?2:((wave>1 && rand()%3==0)?1:0);
        e.hp=e.type==2?115:(e.type==1?58:34);e.speed=e.type==2?2.0f:(e.type==1?3.0f:4.1f);
        e.radius=e.type==2?.85f:.55f;e.attack=e.type==2?24:12;e.phase=rnd(0,6.28f);e.alive=true;enemies.push_back(e);
    }
    if(wave%3==0){Pickup p;p.p=V3(rnd(-8,8),.5f,rnd(-8,8));p.type=2;p.spin=0;p.alive=true;pickups.push_back(p);}
    notice="WAVE "+std::to_string(wave)+"  //  HOSTILES INBOUND";noticeTime=2.6f;
}
V3 forwardDir(){return norm(V3(sinf(yaw),0,-cosf(yaw)));}
V3 rightDir(){return norm(V3(cosf(yaw),0,sinf(yaw)));}
void fire(){
    if(state!=PLAYING||reloadTimer>0||fireCooldown>0)return;
    if(ammo<=0){reloadTimer=1.2f;notice="RELOADING";noticeTime=1;return;}
    V3 f=forwardDir(); float spread=weapon==SHOTGUN?.12f:0;
    int count=weapon==SHOTGUN?7:1;
    for(int i=0;i<count;i++){
        float a=yaw+rnd(-spread,spread),sp=weapon==RAIL?65:(weapon==SHOTGUN?34:48);
        Bullet b;b.p=player+V3(0,1.25f,0)+V3(sinf(a)*.7f,0,-cosf(a)*.7f);
        b.v=V3(sinf(a)*sp,0,-cosf(a)*sp);b.life=weapon==RAIL?1.2f:1.0f;
        b.damage=weapon==RAIL?95:(weapon==SHOTGUN?13:24);b.hostile=false;bullets.push_back(b);
    }
    ammo--;fireCooldown=weapon==SHOTGUN?.72f:(weapon==RAIL?1.0f:.16f);muzzle=.07f;
    if(ammo==0){notice="MAGAZINE EMPTY - PRESS R";noticeTime=1.5f;}
}
void reload(){if(reloadTimer<=0&&ammo<magSize&&reserveAmmo>0){reloadTimer=weapon==SHOTGUN?1.35f:1.15f;}}
void switchWeapon(int n){
    weapon=(Weapon)n;
    magSize=weapon==PULSE?30:(weapon==SHOTGUN?8:5);
    ammo=std::min(ammo,magSize);fireCooldown=.2f;
    notice=weapon==PULSE?"PULSE RIFLE":(weapon==SHOTGUN?"ARC SHOTGUN":"RAIL LANCE");noticeTime=1.4f;
}
void update(float dt){
    if(state!=PLAYING)return;
    dt=std::min(dt,.04f);gameTime+=dt;
    fireCooldown=std::max(0.0f,fireCooldown-dt);dashCooldown=std::max(0.0f,dashCooldown-dt);
    invuln=std::max(0.0f,invuln-dt);muzzle=std::max(0.0f,muzzle-dt);hitFlash=std::max(0.0f,hitFlash-dt);
    noticeTime=std::max(0.0f,noticeTime-dt);shake=std::max(0.0f,shake-dt*2);
    if(reloadTimer>0){reloadTimer-=dt;if(reloadTimer<=0){int need=magSize-ammo,got=std::min(need,reserveAmmo);ammo+=got;reserveAmmo-=got;}}
    energy=std::min(100.0f,energy+dt*13);
    if(dashTime>0)dashTime-=dt;
    V3 move;
    if(keys['w']||keys['W'])move=move+forwardDir();
    if(keys['s']||keys['S'])move=move-forwardDir();
    if(keys['d']||keys['D'])move=move+rightDir();
    if(keys['a']||keys['A'])move=move-rightDir();
    float speed=(sprinting?10.0f:6.2f);
    if(len(move)>0.01f)move=norm(move)*(speed*(dashTime>0?2.8f:1.0f)*dt);
    V3 next=player+move;
    if(!blocked(V3(next.x,0,player.z)))player.x=next.x;
    if(!blocked(V3(player.x,0,next.z)))player.z=next.z;
    player.x=clampf(player.x,-29,29);player.z=clampf(player.z,-29,29);
    if(keys[' ']&&dashCooldown<=0&&energy>=25){dashTime=.18f;dashCooldown=1.25f;energy-=25;invuln=.22f;burst(player,12,.1f,.8f,1);}
    if(mouseDown)fire();
    if(wave==0)waveDelay-=dt;
    if(wave==0&&waveDelay<=0)spawnWave();
    if(wave>0){
        bool any=false;for(size_t i=0;i<enemies.size();i++)if(enemies[i].alive)any=true;
        if(!any){waveDelay-=dt;if(waveDelay<=0){waveDelay=3.0f;spawnWave();}}
    }
    for(size_t i=0;i<enemies.size();i++){
        Enemy &e=enemies[i];if(!e.alive)continue;
        e.phase+=dt*2;
        V3 d=player-e.p;float l=len(d);
        if(l>1.5f){
            V3 step=norm(d)*(e.speed*dt);
            V3 np=e.p+step;
            if(!blocked(np))e.p=np;
            else {float side=(e.phase>3?1:-1);e.p=e.p+V3(-step.z*side,0,step.x*side);}
        }else if(invuln<=0){
            float dmg=e.attack*dt;
            if(shield>0){float take=std::min(shield,dmg);shield-=take;dmg-=take;}
            health-=dmg;if(dmg>0)hitFlash=.15f;
            if(health<=0){health=0;state=DEAD;notice="SIGNAL LOST";noticeTime=10;}
        }
    }
    for(size_t i=0;i<bullets.size();i++){
        Bullet &b=bullets[i];b.p=b.p+b.v*dt;b.life-=dt;
        if(!b.hostile){
            for(size_t j=0;j<enemies.size();j++){
                Enemy &e=enemies[j];if(!e.alive)continue;
                if(dist(b.p,e.p+V3(0,.8f,0))<e.radius+.35f){
                    e.hp-=b.damage;b.life=0;burst(b.p,4,.1f,.8f,1);
                    if(e.hp<=0){e.alive=false;kills++;score+=e.type==2?350:(e.type==1?180:100);burst(e.p+V3(0,1,0),22,1,.15f,.55f);
                        if(rand()%100<18){Pickup p;p.p=e.p;p.type=rand()%2;p.spin=0;p.alive=true;pickups.push_back(p);}
                    }break;
                }
            }
        }
    }
    bullets.erase(std::remove_if(bullets.begin(),bullets.end(),[](Bullet b){return b.life<=0;}),bullets.end());
    for(size_t i=0;i<pickups.size();i++){
        Pickup &p=pickups[i];if(!p.alive)continue;p.spin+=dt*2;
        if(dist(player,p.p)<1.5f){p.alive=false;if(p.type==0){health=std::min(100.0f,health+30);notice="NANOMED +30";}else if(p.type==1){shield=std::min(100.0f,shield+35);notice="SHIELD CELL +35";}else{ammo+=magSize*2;reserveAmmo+=magSize*2;notice="SUPPLY CACHE";}noticeTime=1.5f;burst(p.p,12,.1f,1,.4f);}
    }
    for(size_t i=0;i<particles.size();i++){Particle &p=particles[i];p.p=p.p+p.v*dt;p.v.y-=5*dt;p.life-=dt;}
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](Particle p){return p.life<=0;}),particles.end());
}
void cube(float x,float y,float z,float sx,float sy,float sz){
    glPushMatrix();glTranslatef(x,y,z);glScalef(sx,sy,sz);glutSolidCube(1);glPopMatrix();
}
void drawArena(){
    glDisable(GL_LIGHTING);
    glColor3f(.025f,.045f,.09f);glBegin(GL_QUADS);
    glVertex3f(-32,0,-32);glVertex3f(32,0,-32);glVertex3f(32,0,32);glVertex3f(-32,0,32);glEnd();
    glLineWidth(1);glColor4f(.05f,.35f,.48f,.55f);glBegin(GL_LINES);
    for(int i=-30;i<=30;i+=2){glVertex3f((float)i,.015f,-30);glVertex3f((float)i,.015f,30);glVertex3f(-30,.015f,(float)i);glVertex3f(30,.015f,(float)i);}glEnd();
    glColor3f(.05f,.5f,.75f);glBegin(GL_LINE_LOOP);for(int i=0;i<100;i++){float a=i*2*PI/100;glVertex3f(cosf(a)*30,.04f,sinf(a)*30);}glEnd();
    for(size_t i=0;i<obstacles.size();i++){
        Obstacle o=obstacles[i];glColor3f(.07f,.13f,.2f);cube(o.p.x,o.sy/2,o.p.z,o.sx,o.sy,o.sz);
        glColor3f(.05f,.75f,1);glBegin(GL_LINE_LOOP);
        float x=o.sx/2,z=o.sz/2,y=o.sy+.015f;
        glVertex3f(o.p.x-x,y,o.p.z-z);glVertex3f(o.p.x+x,y,o.p.z-z);glVertex3f(o.p.x+x,y,o.p.z+z);glVertex3f(o.p.x-x,y,o.p.z+z);glEnd();
    }
    glEnable(GL_LIGHTING);
}
void drawPlayer(){
    glPushMatrix();glTranslatef(player.x,0,player.z);glRotatef(yaw*180/PI,0,1,0);
    glColor3f(.05f,.75f,1);glutSolidSphere(.42,16,12);
    glColor3f(.12f,.2f,.3f);cube(0,.9f,0,.65f,1.0f,.42f);
    glColor3f(.1f,.9f,1);cube(0,1.0f,-.23f,.35f,.25f,.08f);
    glColor3f(.2f,.35f,.48f);cube(-.42f,.85f,0,.18f,.65f,.2f);cube(.42f,.85f,0,.18f,.65f,.2f);
    glColor3f(.8f,.95f,1);cube(0,.42f,-.08f,.16f,.25f,.18f);
    glPopMatrix();
}
void drawEnemies(){
    for(size_t i=0;i<enemies.size();i++){Enemy e=enemies[i];if(!e.alive)continue;
        glPushMatrix();glTranslatef(e.p.x,.7f+sin(e.phase)*.12f,e.p.z);
        if(e.type==2){glColor3f(.75f,.08f,.2f);glutSolidSphere(.9,18,14);glColor3f(1,.3f,.2f);cube(0,1.0f,0,.9f,.25f,.9f);}
        else if(e.type==1){glColor3f(.7f,.2f,1);glutSolidOctahedron();}
        else{glColor3f(1,.18f,.28f);glutSolidSphere(.52,14,12);}
        glColor3f(1,.8f,.8f);glPushMatrix();glTranslatef(0,.12f,-.42f);glutSolidSphere(.12,8,8);glPopMatrix();
        glPopMatrix();
        glDisable(GL_LIGHTING);glColor3f(.15f,.02f,.04f);glBegin(GL_QUADS);glVertex3f(e.p.x-.5f,1.8f,e.p.z);glVertex3f(e.p.x+.5f,1.8f,e.p.z);glVertex3f(e.p.x+.5f+0,1.88f,e.p.z);glVertex3f(e.p.x-.5f,1.88f,e.p.z);glEnd();
        glColor3f(1,.1f,.2f);glBegin(GL_QUADS);glVertex3f(e.p.x-.5f,1.81f,e.p.z);glVertex3f(e.p.x-.5f+clampf(e.hp/(e.type==2?115:(e.type==1?58:34)),0,1),1.81f,e.p.z);glVertex3f(e.p.x-.5f+clampf(e.hp/(e.type==2?115:(e.type==1?58:34)),0,1),1.87f,e.p.z);glVertex3f(e.p.x-.5f,1.87f,e.p.z);glEnd();glEnable(GL_LIGHTING);
    }
}
void drawWorldEffects(){
    for(size_t i=0;i<bullets.size();i++){Bullet b=bullets[i];glPushMatrix();glTranslatef(b.p.x,b.p.y,b.p.z);glColor3f( b.hostile?1:.1f,b.hostile?.2f:.85f,1);glutSolidSphere(.12,8,8);glPopMatrix();}
    for(size_t i=0;i<pickups.size();i++){Pickup p=pickups[i];if(!p.alive)continue;glPushMatrix();glTranslatef(p.p.x,.6f+sin(p.spin*2)*.15f,p.p.z);glRotatef(p.spin*80,0,1,0);glColor3f(p.type==0?.1f:1,p.type==1?.8f:.2f,p.type==2?.2f:.1f);glutSolidOctahedron();glPopMatrix();}
    glDisable(GL_LIGHTING);glPointSize(4);glBegin(GL_POINTS);for(size_t i=0;i<particles.size();i++){Particle p=particles[i];glColor4f(p.r,p.g,p.b,p.life/p.maxLife);glVertex3f(p.p.x,p.p.y,p.p.z);}glEnd();glEnable(GL_LIGHTING);
}
void drawHUD(){
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_LIGHTING);glDisable(GL_DEPTH_TEST);
    // reticle
    float cx=W/2,cy=H/2;glColor3f(.25f,1,1);glLineWidth(2);glBegin(GL_LINES);
    glVertex2f(cx-14,cy);glVertex2f(cx-5,cy);glVertex2f(cx+5,cy);glVertex2f(cx+14,cy);
    glVertex2f(cx,cy-14);glVertex2f(cx,cy-5);glVertex2f(cx,cy+5);glVertex2f(cx,cy+14);glEnd();
    text(28,H-38,"NEON VANGUARD  /  RIFT PROTOCOL",GLUT_BITMAP_HELVETICA_18);
    text(28,H-64,"WAVE "+std::to_string(wave)+"     ELIMINATIONS "+std::to_string(kills)+"     SCORE "+std::to_string(score));
    text(28,70,"VITALS");bar(28,48,210,15,health,100,1,.12f,.22f);
    text(28,25,"SHIELD");bar(100,25,138,12,shield,100,.1f,.55f,1);
    text(28,105,"ENERGY");bar(100,105,138,12,energy,100,.1f,1,.75f);
    std::string wn=weapon==PULSE?"PULSE RIFLE":(weapon==SHOTGUN?"ARC SHOTGUN":"RAIL LANCE");
    text(W-260,H-42,wn);text(W-260,H-70,"AMMO  "+std::to_string(ammo)+" / "+std::to_string(reserveAmmo));
    text(W-260,H-98,"[1] [2] [3]  WEAPON SELECT");
    text(W-260,34,"WASD MOVE   SPACE DASH   R RELOAD");
    if(noticeTime>0){textCenter(W/2,H-125,notice,GLUT_BITMAP_HELVETICA_18);}
    if(reloadTimer>0)textCenter(W/2,cy-55,"RELOADING...",GLUT_BITMAP_HELVETICA_18);
    if(hitFlash>0){quad(0,0,W,H,1,.02f,.04f,hitFlash*1.4f);}
    glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);
}
void overlay(const std::string&title,const std::string&sub,const std::string&foot){
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();gluOrtho2D(0,W,0,H);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glDisable(GL_LIGHTING);glDisable(GL_DEPTH_TEST);
    quad(0,0,W,H,.005f,.012f,.035f,.92f);
    textCenter(W/2,H*.68f,title,GLUT_BITMAP_TIMES_ROMAN_24);
    textCenter(W/2,H*.55f,sub,GLUT_BITMAP_HELVETICA_18);
    textCenter(W/2,H*.40f,foot,GLUT_BITMAP_HELVETICA_18);
    textCenter(W/2,H*.25f,"NEON VANGUARD  //  RIFT PROTOCOL",GLUT_BITMAP_HELVETICA_12);
    glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);
    glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);
}
void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();gluPerspective(62,(float)W/H,.1,120);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    V3 f=forwardDir();V3 target=player+V3(0,1.1f,0);
    V3 eye=player-f*10+V3(0,5.5f,0);
    gluLookAt(eye.x,eye.y,eye.z,target.x,target.y,target.z,0,1,0);
    GLfloat lightPos[]={0,18,0,1};glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
    drawArena();drawWorldEffects();drawEnemies();drawPlayer();
    if(state==PLAYING)drawHUD();
    if(state==TITLE)overlay("NEON VANGUARD","A hostile rift has opened. You are the last containment operative.","PRESS ENTER TO DEPLOY   |   WASD MOVE  MOUSE AIM  CLICK FIRE");
    if(state==PAUSED)overlay("SYSTEM PAUSED","Your containment suit is holding position.","PRESS P OR ESC TO RESUME");
    if(state==DEAD)overlay("SIGNAL LOST","FINAL SCORE: "+std::to_string(score)+"   |   WAVES: "+std::to_string(wave),"PRESS ENTER TO REDEPLOY");
    if(state==VICTORY)overlay("RIFT CONTAINED","You survived the breach.","PRESS ENTER TO PLAY AGAIN");
    glutSwapBuffers();
}
void reshape(int w,int h){glViewport(0,0,w,h);}
void keyDown(unsigned char k,int,int){
    keys[k]=true;
    sprinting=(glutGetModifiers() & GLUT_ACTIVE_SHIFT)!=0;
    if(k==27||k=='p'||k=='P'){if(state==PLAYING)state=PAUSED;else if(state==PAUSED)state=PLAYING;}
    if(k==13&&(state==TITLE||state==DEAD||state==VICTORY))resetGame();
    if(k=='r'||k=='R')reload();
    if(k=='1')switchWeapon(PULSE);
    if(k=='2')switchWeapon(SHOTGUN);
    if(k=='3')switchWeapon(RAIL);
}
void keyUp(unsigned char k,int,int){keys[k]=false;sprinting=(glutGetModifiers() & GLUT_ACTIVE_SHIFT)!=0;}
void specialDown(int k,int,int){specialKeys[k]=true;}
void specialUp(int k,int,int){specialKeys[k]=false;}
void mouseMove(int x,int y){
    if(state==PLAYING){float dx=x-W/2,dy=y-H/2;yaw+=dx*.0028f;pitch=clampf(pitch-dy*.0015f,-.15f,.75f);glutWarpPointer(W/2,H/2);}
    mouseX=x;mouseY=y;
}
void mouseButton(int button,int s,int,int){
    if(button==GLUT_LEFT_BUTTON)mouseDown=(s==GLUT_DOWN);
}
int lastTime=0;
void timer(int){
    int now=glutGet(GLUT_ELAPSED_TIME);float dt=(now-lastTime)/1000.0f;lastTime=now;
    update(dt);glutPostRedisplay();glutTimerFunc(16,timer,0);
}
void initGL(){
    glClearColor(.005f,.01f,.025f,1);glEnable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_COLOR_MATERIAL);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    GLfloat amb[]={.25f,.3f,.42f,1},diff[]={.8f,.9f,1,1};glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);glLightfv(GL_LIGHT0,GL_DIFFUSE,diff);
    glShadeModel(GL_SMOOTH);glEnable(GL_NORMALIZE);
}
int main(int argc,char**argv){
    srand((unsigned int)time(0));glutInit(&argc,argv);glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(W,H);glutCreateWindow("NEON VANGUARD: RIFT PROTOCOL");
    initGL();buildArena();glutDisplayFunc(display);glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);glutKeyboardUpFunc(keyUp);glutSpecialFunc(specialDown);glutSpecialUpFunc(specialUp);
    glutMouseFunc(mouseButton);glutPassiveMotionFunc(mouseMove);glutMotionFunc(mouseMove);
    glutSetCursor(GLUT_CURSOR_NONE);lastTime=glutGet(GLUT_ELAPSED_TIME);glutTimerFunc(16,timer,0);
    glutMainLoop();return 0;
}
