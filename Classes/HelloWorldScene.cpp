
#include <PluginSdkboxPlay/PluginSdkboxPlay.h>
#include "HelloWorldScene.h"
#include "SimpleAudioEngine.h"
#include "ui/CocosGUI.h"
#ifdef SDKBOX_ENABLED
#include "PluginGoogleAnalytics/PluginGoogleAnalytics.h"
#endif
using namespace CocosDenshion;

USING_NS_CC;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#define SPACE_GAME "SpaceGame.caf"
#define EXPLOSION_LARGE "explosion_large.caf"
#define LASER_SHIP "laser_ship.caf"
#else
#define SPACE_GAME "BackgroundMusic.mp3"
#define EXPLOSION_LARGE "explosion_large.wav"
#define LASER_SHIP "laser_ship.wav"
#endif


enum class PhysicsCategory { //create a physics enum class  with appropriate identifiers
    None = 0,
    Monster = (1 << 0),    // 1
    Projectile = (1 << 1), // 2
    All = PhysicsCategory::Monster | PhysicsCategory::Projectile // 3
};
Scene* HelloWorld::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::createWithPhysics(); //create a scene with physics to use the physics collision deetction
    scene->getPhysicsWorld()->setGravity(Vec2(0,0)); //set physics gravity
    scene->getPhysicsWorld()->setDebugDrawMask(PhysicsWorld::DEBUGDRAW_ALL);

    // 'layer' is an autorelease object
    auto layer = HelloWorld::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{

    // super init first
    if ( !Layer::init() )
    {
        return false;
    }
	sdkbox::PluginSdkboxPlay::init(); //initialise plugin with leaderboard/achievements from json config
    sdkbox::PluginSdkboxPlay::signin(); //sign into google play account
    Size visibleSize = Director::getInstance()->getVisibleSize();
    Point origin = Director::getInstance()->getVisibleOrigin();

	// add a "close" icon to exit the progress. it's an autorelease object
	auto closeItem = MenuItemImage::create(
		"CloseNormal.png",
		"CloseSelected.png",
		CC_CALLBACK_1(HelloWorld::menuCloseCallback, this));

	closeItem->setPosition(Point(origin.x + visibleSize.width - closeItem->getContentSize().width / 2,
		origin.y + closeItem->getContentSize().height / 2));

	// create menu, it's an autorelease object

	auto menu = Menu::create(closeItem, NULL);
	menu->setPosition(Point::ZERO);
	this->addChild(menu, 1);


	//GALAXY

	_batchNode = SpriteBatchNode::create("Sprites.pvr.ccz");
	this->addChild(_batchNode);

	SpriteFrameCache::getInstance()->addSpriteFramesWithFile("Sprites.plist");

	_ship = Sprite::createWithSpriteFrameName("SpaceFlier_sm_1.png");
	_ship->setTag(3); //ship is tagged as 3
    auto projectileSize = _ship->getContentSize();
    auto physicsBody = PhysicsBody::createCircle(projectileSize.width/2 );
    physicsBody->setDynamic(true);
    physicsBody->setCategoryBitmask((int)PhysicsCategory::Projectile);
    physicsBody->setCollisionBitmask((int)PhysicsCategory::None);
    physicsBody->setContactTestBitmask((int)PhysicsCategory::Monster);
    _ship->setPhysicsBody(physicsBody);
	_ship->setPosition(visibleSize.width * 0.1, visibleSize.height * 0.5);
	_batchNode->addChild(_ship, 1);

	// 1) Create the ParallaxNode
	_backgroundNode = ParallaxNodeExtras::create();
	this->addChild(_backgroundNode, -1);

	// 2) Create the sprites will be added to the ParallaxNode
	_spaceDust1 = Sprite::create("bg_front_spacedust.png");
	_spaceDust2 = Sprite::create("bg_front_spacedust.png");
	_planetSunrise = Sprite::create("bg_planetsunrise.png");
	_galaxy = Sprite::create("bg_galaxy.png");
	_spatialAnomaly1 = Sprite::create("bg_spacialanomaly.png");
	_spatialAnomaly2 = Sprite::create("bg_spacialanomaly2.png");

	// 3) Determine relative movement speeds for space dust and background
	auto dustSpeed = Point(0.1F, 0.1F);
	auto bgSpeed = Point(0.05F, 0.05F);

	// 4) Add children to ParallaxNode
	_backgroundNode->addChild(_spaceDust1, 0, dustSpeed, Point(0, visibleSize.height / 2));
	_backgroundNode->addChild(_spaceDust2, 0, dustSpeed, Point(_spaceDust1->getContentSize().width, visibleSize.height / 2));
	_backgroundNode->addChild(_galaxy, -1, bgSpeed, Point(0, visibleSize.height * 0.7));
	_backgroundNode->addChild(_planetSunrise, -1, bgSpeed, Point(600, visibleSize.height * 0));
	_backgroundNode->addChild(_spatialAnomaly1, -1, bgSpeed, Point(900, visibleSize.height * 0.3));
	_backgroundNode->addChild(_spatialAnomaly2, -1, bgSpeed, Point(1500, visibleSize.height * 0.9));

	HelloWorld::addChild(ParticleSystemQuad::create("Stars1.plist"));
	HelloWorld::addChild(ParticleSystemQuad::create("Stars2.plist"));
	HelloWorld::addChild(ParticleSystemQuad::create("Stars3.plist"));

#define KNUMASTEROIDS 15
	_asteroids = new Vector<Sprite*>(KNUMASTEROIDS);
	for (int i = 0; i < KNUMASTEROIDS; ++i) {
		asteroid = Sprite::createWithSpriteFrameName("asteroid.png");
		asteroid->setVisible(false);
		_batchNode->addChild(asteroid);
		_asteroids->pushBack(asteroid);
	}


#define KNUMLASERS 5
	_shipLasers = new Vector<Sprite*>(KNUMLASERS);
	for (int i = 0; i < KNUMLASERS; ++i) {
		auto shipLaser = Sprite::createWithSpriteFrameName("laserbeam_blue.png");
		shipLaser->setVisible(false);
		_batchNode->addChild(shipLaser);
		_shipLasers->pushBack(shipLaser);
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Layer::setAccelerometerEnabled(true);
	auto accelerationListener = EventListenerAcceleration::create(CC_CALLBACK_2(HelloWorld::onAcceleration, this));
	_eventDispatcher->addEventListenerWithSceneGraphPriority(accelerationListener, this);
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////


	auto touchListener = EventListenerTouchAllAtOnce::create();
	touchListener->onTouchesBegan = CC_CALLBACK_2(HelloWorld::onTouchesBegan, this);
	_eventDispatcher->addEventListenerWithSceneGraphPriority(touchListener, this);

	_lives = 3;
	double curTime = getTimeTick();
	_gameOverTime = curTime + 120000; //change the games play time

	this->scheduleUpdate();

	score = 0;

	SimpleAudioEngine::getInstance()->playBackgroundMusic(SPACE_GAME, true);
	SimpleAudioEngine::getInstance()->preloadEffect(EXPLOSION_LARGE);
	SimpleAudioEngine::getInstance()->preloadEffect(LASER_SHIP);
	srand((unsigned int)time(nullptr));
	this->schedule(schedule_selector(HelloWorld::addMonster), 1.5);
    auto contactListener = EventListenerPhysicsContact::create();
    contactListener->onContactBegin = CC_CALLBACK_1(HelloWorld::onContactBegan, this);
    this->getEventDispatcher()->addEventListenerWithSceneGraphPriority(contactListener, this);

    scoreLabel = Label::createWithSystemFont("Score: 0", "Calibri",30.0f); //create a label for score
    scoreLabel->setColor(Color3B::WHITE); // set label colour
    scoreLabel->setAnchorPoint({0,0}); // set anchor point to origin
    scoreLabel->setPosition(Vec2(200,100));//set it's x and y coordinates
    scoreLabel->setScale(2,2);//set scale of label
    this->addChild(scoreLabel); //add as child of batchnode

    livesLabel = Label::createWithSystemFont("Lives: 3", "Calibri",30.0f); //create label for lives
    livesLabel->setColor(Color3B::WHITE); //set colour
    livesLabel->setAnchorPoint({0,0}); //set its anchor point
    livesLabel->setPosition(Vec2(600,100)); // set it's position
    livesLabel->setScale(2,2); //set the scale of it
    this->addChild(livesLabel); //add it as a child of the batchnode

	return true;
}

void HelloWorld::update(float dt)
{
	auto backgroundScrollVert = Point(-1000, 0);
	_backgroundNode->setPosition(_backgroundNode->getPosition() + (backgroundScrollVert * dt));

	//my code
    scoreLabel->setString("Score: " + StringUtils::format("%d",score)); //update the score and lives labels
    livesLabel->setString("Lives: " + StringUtils::format("%d",_lives));

    //****************************************************************


	//Parallax
	auto spaceDusts = new Vector<Sprite*>(2);
	spaceDusts->pushBack(_spaceDust1);
	spaceDusts->pushBack(_spaceDust2);
	for (auto spaceDust : *spaceDusts) {
		float xPosition = _backgroundNode->convertToWorldSpace(spaceDust->getPosition()).x;
		float size = spaceDust->getContentSize().width;
		if (xPosition < -size / 2) {
			_backgroundNode->incrementOffset(Point(spaceDust->getContentSize().width * 2, 0), spaceDust);
		}
	}

	auto backGrounds = new Vector<Sprite*>(4);
	backGrounds->pushBack(_galaxy);
	backGrounds->pushBack(_planetSunrise);
	backGrounds->pushBack(_spatialAnomaly1);
	backGrounds->pushBack(_spatialAnomaly2);
	for (auto background : *backGrounds) {
		float xPosition = _backgroundNode->convertToWorldSpace(background->getPosition()).x;
		float size = background->getContentSize().width;
		if (xPosition < -size) {
			_backgroundNode->incrementOffset(Point(2000, 0), background);
		}
	}



	//Acceleration
	Size winSize = Director::getInstance()->getWinSize();
	float maxY = winSize.height - _ship->getContentSize().height / 2;
	float minY = _ship->getContentSize().height / 2;
	float diff = (_shipPointsPerSecY * dt);
	float newY = _ship->getPosition().y + diff;
	newY = MIN(MAX(newY, minY), maxY);
	_ship->setPosition(_ship->getPosition().x, newY);




	float curTimeMillis = getTimeTick();
	if (curTimeMillis > _nextAsteroidSpawn) {

		float randMillisecs = randomValueBetween(0.20F, 1.0F) * 1200;
		_nextAsteroidSpawn = randMillisecs + curTimeMillis;

		float randY = randomValueBetween(0.0F, winSize.height);
		float randDuration = randomValueBetween(2.0F, 10.0F);

		Sprite *asteroid = _asteroids->at(_nextAsteroid);
		_nextAsteroid++;

		if (_nextAsteroid >= _asteroids->size())
			_nextAsteroid = 0;

		asteroid->stopAllActions();
		asteroid->setPosition(winSize.width + asteroid->getContentSize().width / 2, randY);
		asteroid->setVisible(true);
		asteroid->runAction(
			Sequence::create(
			MoveBy::create(randDuration, Point(-winSize.width - asteroid->getContentSize().width, 0)),
			CallFuncN::create(CC_CALLBACK_1(HelloWorld::setInvisible, this)),
			NULL /* DO NOT FORGET TO TERMINATE WITH NULL (unexpected in C++)*/)
			);
	}
	// Asteroids
	for (auto asteroid : *_asteroids){
		if (!(asteroid->isVisible()))
			continue;
		for (auto shipLaser : *_shipLasers){
			if (!(shipLaser->isVisible()))
				continue;
			if (shipLaser->getBoundingBox().intersectsRect(asteroid->getBoundingBox())){
				SimpleAudioEngine::getInstance()->playEffect(EXPLOSION_LARGE);
				shipLaser->setVisible(false);
                shipLaser->removeComponent(shipLaser->getPhysicsBody( ));
                asteroid->removeComponent(asteroid->getPhysicsBody( ));
                killCount++;
                if(killCount==1){
                    sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "First blood", 10);
                    sdkbox::PluginSdkboxPlay::unlockAchievement("First blood");
                }
                if(killCount==25){
                    sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "Massacre", 70);
                    sdkbox::PluginSdkboxPlay::unlockAchievement("Massacre");
                }
				asteroid->setVisible(false);

				score = score + 10;
                if(score>=300){
                    sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "Top scorer", 100);
                    sdkbox::PluginSdkboxPlay::unlockAchievement("TopScorer");
                }
			}

		}
		if (_ship->getBoundingBox().intersectsRect(asteroid->getBoundingBox())){
			asteroid->setVisible(false);
            SimpleAudioEngine::getInstance()->playEffect(EXPLOSION_LARGE);
			_ship->runAction(Blink::create(1.0F, 9));
			_lives--;
		}
	}

	if (_lives <= 0) {
        _lives=0;
		_ship->stopAllActions();
		_ship->setVisible(false);
        //_ship->removeComponent(_ship->getPhysicsBody( ));
		this->endScene(KENDREASONLOSE);
	}
	else if (curTimeMillis >= _gameOverTime) {
		this->endScene(KENDREASONWIN);
	}
}

void HelloWorld::onAcceleration(Acceleration* acc, Event* event)
{
#define KFILTERINGFACTOR 0.1
#define KRESTACCELX -0.6
#define KSHIPMAXPOINTSPERSEC (winSize.height*0.5)
#define KMAXDIFFX 0.2

	double rollingX = 0;

	// Cocos2DX inverts X and Y accelerometer depending on device orientation
	// in landscape mode right x=-y and y=x !!! (Strange and confusing choice)
	acc->x = acc->y;
	rollingX = (acc->x * KFILTERINGFACTOR) + (rollingX * (1.0 - KFILTERINGFACTOR));
	double accelX = acc->x - rollingX;
	Size winSize = Director::getInstance()->getWinSize();
	double accelDiff = accelX - KRESTACCELX;
	double accelFraction = accelDiff / KMAXDIFFX;
	_shipPointsPerSecY = KSHIPMAXPOINTSPERSEC * accelFraction;
}

float HelloWorld::randomValueBetween(float low, float high)
{
	// from http://stackoverflow.com/questions/686353/c-random-float-number-generation
	return low + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (high - low)));
}

float HelloWorld::getTimeTick() {
	timeval time;
	gettimeofday(&time, NULL);
	unsigned long millisecs = (time.tv_sec * 1000) + (time.tv_usec / 1000);
	return (float)millisecs;
}

void HelloWorld::setInvisible(Node * node) {
	node->setVisible(false);
}

void HelloWorld::onTouchesBegan(const std::vector<Touch*>& touches, Event  *event){
	SimpleAudioEngine::getInstance()->playEffect(LASER_SHIP);
	auto winSize = Director::getInstance()->getWinSize();
	auto shipLaser = _shipLasers->at(_nextShipLaser++);
	shipLaser->setTag(1); //set ships tag to 1
	if (_nextShipLaser >= _shipLasers->size())
		_nextShipLaser = 0;
	shipLaser->setPosition(_ship->getPosition() + Point(shipLaser->getContentSize().width / 2, 0));
    auto projectileSize = shipLaser->getContentSize();
    sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "Firearms operational", 10);
    sdkbox::PluginSdkboxPlay::unlockAchievement("FirearmsOperational");

    //creating the physics body and settings its bitmask
    auto physicsBody = PhysicsBody::createCircle(projectileSize.width/2 );
    physicsBody->setDynamic(true);
    physicsBody->setCategoryBitmask((int)PhysicsCategory::Projectile);
    physicsBody->setCollisionBitmask((int)PhysicsCategory::None);
    physicsBody->setContactTestBitmask((int)PhysicsCategory::Monster);
    shipLaser->setPhysicsBody(physicsBody); //add physics body to ships laser


	shipLaser->setVisible(true);
	shipLaser->stopAllActions();
	shipLaser->runAction(
		Sequence::create(
		MoveBy::create(0.5, Point(winSize.width, 0)), 
		CallFuncN::create(CC_CALLBACK_1(HelloWorld::setInvisible, this)),
		NULL));

}

void HelloWorld::restartTapped(Ref* pSender) {
	Director::getInstance()->replaceScene
		(TransitionZoomFlipX::create(0.5, this->createScene()));
	// reschedule
	this->scheduleUpdate();
}

void HelloWorld::endScene(EndReason endReason) {

	sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "Game Finished", 5);
    sdkbox::PluginSdkboxPlay::submitScore("Leaderboard_my_leaderboard",score);
    sdkbox::PluginSdkboxPlay::unlockAchievement("GameFinished");

	if (_gameOver)
		return;
	_gameOver = true;

	auto winSize = Director::getInstance()->getWinSize();
	char message[10] = "You Win";
	if (endReason == KENDREASONLOSE)

		strcpy(message, "You Lose");
	auto label = Label::createWithBMFont("Arial.fnt", message);
	label->setScale(0.1F);
	label->setPosition(winSize.width / 2, winSize.height*0.6F);
	this->addChild(label);

	strcpy(message, "Restart");
	auto restartLabel = Label::createWithBMFont("Arial.fnt", message);
	auto restartItem = MenuItemLabel::create(restartLabel, CC_CALLBACK_1(HelloWorld::restartTapped, this));
	restartItem->setScale(0.1F);
	restartItem->setPosition(winSize.width / 2, winSize.height*0.4);

	auto *menu = Menu::create(restartItem, NULL);
	menu->setPosition(Point::ZERO);
	this->addChild(menu);

	// clear label and menu
	restartItem->runAction(ScaleTo::create(0.5F, 1.0F));
	label->runAction(ScaleTo::create(0.5F, 1.0F));
	
	// Terminate update callback
	this->unscheduleUpdate();
}

//--------------------------------------------------------------- my code
void HelloWorld::addMonster(float dt) {
    monster = Sprite::create("monster.png"); // create a monster sprite
    monster->setTag(2); //set the tag of value 2
    auto monsterSize = monster->getContentSize(); //get size of the monster
    auto physicsBody = PhysicsBody::createBox(Size(monsterSize.width , monsterSize.height), //create a physics body around our monster
                                              PhysicsMaterial(0.1f, 1.0f, 0.0f));
// 2
    physicsBody->setDynamic(true); //physics engine does not add force to this object
// 3
    physicsBody->setCategoryBitmask((int)PhysicsCategory::Monster);
    physicsBody->setCollisionBitmask((int)PhysicsCategory::None);        // set the bitmasks
    physicsBody->setContactTestBitmask((int)PhysicsCategory::Projectile);
    monster->setScale(1.5,1.5); //set the monsters size
    monster->setPhysicsBody(physicsBody); //give the monster the physics body we created
    // 1
    auto monsterContentSize = monster->getContentSize();
    auto selfContentSize = this->getContentSize(); //get sizes of monster and screen
    int minY = monsterContentSize.height/2;
    int maxY = selfContentSize.height - monsterContentSize.height/2;
    int rangeY = maxY - minY;
    int randomY = (rand() % rangeY) + minY;

    monster->setPosition(Vec2(selfContentSize.width + monsterContentSize.width/2, randomY));//spawn monster in positions relative to screen and monster size
    this->addChild(monster); //add the monster to the game

    // 2
    int minDuration = 2.0;
    int maxDuration = 4.0;
    int rangeDuration = maxDuration - minDuration;
    int randomDuration = (rand() % rangeDuration) + minDuration; //get a random duration for spawning the enemy

    // 3
    auto actionMove = MoveTo::create(randomDuration, Vec2(-monsterContentSize.width/2, randomY));
    auto actionRemove = RemoveSelf::create();
    monster->runAction(Sequence::create(actionMove,actionRemove, nullptr));
}
//------------------------------------------------------------------------------------------------------------

bool HelloWorld::onContactBegan(PhysicsContact &contact) {
    auto a = contact.getShapeA()->getBody()->getNode(); //get nodes that collide
    auto b = contact.getShapeB()->getBody()->getNode();
    // ship laser 1, on monster 2
    if ( ( 1 == a->getTag() && 2  == b->getTag()
         ) || ( 2 == a->getTag() && 1 == b->getTag() ) ) //check node tags
    {

        //if the ships laser hits a monster or vice versa.....
        a->removeComponent(a->getPhysicsBody( ));  //remove their physics bodies
        b->removeComponent(b->getPhysicsBody( ));
        a->setVisible(false); //set them to be invisible
        b->setVisible(false);
        killCount++;
        if(killCount==1){
            sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "First blood", 10);
            sdkbox::PluginSdkboxPlay::unlockAchievement("FirstBlood");
        }
        if(killCount==25){
            sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "Massacre", 70);
            sdkbox::PluginSdkboxPlay::unlockAchievement("Massacre");
        }
        SimpleAudioEngine::getInstance()->playEffect(EXPLOSION_LARGE); //play sound effect
        score = score + 15; //increment score
        if(score>=300){
            sdkbox::PluginGoogleAnalytics::logEvent("Achievement", "Unlocked", "Top scorer", 100);
            sdkbox::PluginSdkboxPlay::unlockAchievement("TopScorer");

        }
    }
    //monster 2, ship 3
    if ( ( 2 == a->getTag() && 3 == b->getTag() ) || ( 3 == a->getTag() && 2 == b->getTag() ) ) //check node tags
    {
        //if the monster hits the ship or vice versa....
        //a->removeComponent(a->getPhysicsBody( ));
        b->removeComponent(b->getPhysicsBody( ));
       // a->setVisible(false);
        b->setVisible(false);
        SimpleAudioEngine::getInstance()->playEffect(EXPLOSION_LARGE); //play sound effect
        score = score + 5; //increment score
        _ship->runAction(Blink::create(1.0F, 9)); //make the ship blink to show damage
        _lives--; //decrement lives
    }
    return true;
}

void HelloWorld::menuCloseCallback(Ref* pSender)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
	MessageBox("You pressed the close button. Windows Store Apps do not implement a close button.","Alert");
    return;
#endif

    Director::getInstance()->end();

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    exit(0);
#endif
}