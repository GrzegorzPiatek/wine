#include "constants.h"

class InterfaceParallel
{
    public:
        virtual void debug();
        virtual bool isMsgFromMyType(Msg);
}