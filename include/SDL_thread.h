#if __cplusplus
extern "C" {
#endif

//This cunning file remaps DOSBox's SDL header include to our custom framework instead
#import <SDL/SDL_thread.h>

#if __cplusplus
} //Extern C
#endif
