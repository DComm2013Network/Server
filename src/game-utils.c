#include "Server.h"

pos_t getLobbyX(playerNo_t plyr){

    return((plyr % 8) * 40) + 440;

}
pos_t getLobbyY(playerNo_t plyr){

    if(plyr < 8){
        return 240;
    }
    if(plyr < 16){
        return 320;
    }
    if(plyr < 24){
        return 400;
    }
    else{
        return 480;
    }
}
