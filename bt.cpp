

#include "threadPool.h"

#include "bt.h"


BT::BT() {
    this->lastState = STATE::OFF;
    this->clickCount = 0;
    this->clickType = CLICK_TYPE::NOCLICK;
}

void BT::process(STATE curState) {
    //STATE curState = this->curState; 
    //std::cout << "Button " << this->getName() << " curState value: " << curState << " btLastValue:" << this->lastState << std::endl;
    if( this->lastState == STATE::OFF && curState == STATE::ON ) {
      //std::cout << "Button " << this->getName() << " curState value: " << state_to_string(curState) << " btLastValue:" << state_to_string(this->lastState) << std::endl;
           //printf("front montant\r\n"); fflush(stdout);
            // *****************************   
            // traitement sur front montant
            // ***************************** 

            this->lastState = STATE::ON;
            
            this->pressedTime = std::chrono::steady_clock::now();

            //this->_onPressed();
            this->_onAction(BUTTON_ACTION::PRESSED, 0);  

        } else if( (this->lastState == STATE::ON) && (curState == STATE::OFF) ) {
          //std::cout << "Button " << this->getName() << " curState value: " << state_to_string(curState) << " btLastValue:" << state_to_string(this->lastState) << std::endl;
            //printf("front descendant\r\n"); fflush(stdout);
            // ***************************** 
            // traitement sur front descendant
            // ***************************** 
            this->lastState = STATE::OFF;
            
            this->releasedTime = std::chrono::steady_clock::now();

           // if (std::chrono::duration_cast<std::chrono::milliseconds>(this->releasedTime - this->pressedTime).count() < this->dcGap) {
                this->clickCount++;
           //     this->clickType = CLICK_TYPE::MULTICLICK;
            //}

            //this->_onReleased(); 
            this->_onAction(BUTTON_ACTION::RELEASED, 0);   

            if (this->clickType == CLICK_TYPE::LONGCLICK) {
                //this->_onLongClickStop();
                this->_onAction(BUTTON_ACTION::LONGCLICKSTOP, 0);
            }
            
        } else if( (this->lastState == STATE::OFF) && (curState == STATE::OFF) ) {
           // printf("front bas\r\n"); fflush(stdout);
            // ***************************** 
            // pas de changement d'état : front bas
            // ***************************** 
            this->t0 = std::chrono::steady_clock::now();
            
            //Attendre DC_Gap pour connaitre le nombre de click      
            if (std::chrono::duration_cast<std::chrono::milliseconds>(this->t0 - this->releasedTime).count() > this->dcGap) {
                if (this->clickType != CLICK_TYPE::LONGCLICK) {
                  if (this->clickCount == 1) {
                    //LOG("SimpleClick");
                    this->clickType = CLICK_TYPE::SIMPLECLICK;
                    
                    //this->_onSimpleClick();
                    this->_onAction(BUTTON_ACTION::SIMPLECLICK, 1);
                  } else if ( this->clickCount > 1 ) {
                      //LOG("MultiClick: %d",this->click_count);
                    this->clickType = CLICK_TYPE::MULTICLICK;
                    
                    //this->_onMultiClicks(this->clickCount);
                    this->_onAction(BUTTON_ACTION::MULTICLICK, this->clickCount);

                  }
              }
                this->clickType = CLICK_TYPE::NOCLICK;
                this->clickCount = 0;
            }
            
        } else if( (this->lastState == STATE::ON) && (curState == STATE::ON) ) {
           // printf("front haut\r\n"); fflush(stdout);
            // ***************************** 
            // pas de changement d'état : front haut
            // ***************************** 

            this->t1 = std::chrono::steady_clock::now();
        
            //long click
            if (std::chrono::duration_cast<std::chrono::milliseconds>(this->t1 - this->pressedTime).count() > this->holdTime && this->clickType != CLICK_TYPE::LONGCLICK) {
                //LOG("LongClick START");
                this->clickType = CLICK_TYPE::LONGCLICK;
                //this->_onLongClickStart();
                this->_onAction(BUTTON_ACTION::LONGCLICKSTART, 0);
            }
        }
}

void BT::_onAction(BUTTON_ACTION btAction, unsigned int clickCount){
  for(auto&& handler : this->actionHandlers) {
    //std::thread(handler, btAction, clickCount).detach();
    threadPool.push_task(handler, btAction, clickCount);
    //handler(btAction, clickCount);
  }
}


void BT::addActionHandler(actionHandlersFunc function) {
    this->actionHandlers.push_back(function);
}

BT::~BT() {}


/*

function delay(delayInms) {
  return new Promise(resolve => {
    setTimeout(() => {
      resolve(2);
    }, delayInms);
  });
}

async function click(nbclick, input = "DI4", d = 50) {
  var delayres;
  for(i=0; i<nbclick; i++) {
    wsClientPublish({"state":"ON"}, "HousePLC/Digital_Inputs/"+input+"/SET",0);
    delayres = await delay(d);
    wsClientPublish({"state":"OFF"}, "HousePLC/Digital_Inputs/"+input+"/SET",0);
    delayres = await delay(d);
  }
  console.log('finished');
}

*/