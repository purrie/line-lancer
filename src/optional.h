#ifndef OPTIONAL_H_
#define OPTIONAL_H_

#define defOptional(name) typedef struct Optional ## name Optional ## name

#define makeOptional(type, name) struct Optional ## name {\
        type value;\
        bool has_value;\
    }

#endif // OPTIONAL_H_
