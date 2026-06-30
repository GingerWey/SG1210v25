//-----------------------------------------------------------------------------
/*
 File        : GFormCentraRegistrar.h
 Version     : V1.00
 By          : Wey. Silver Grid

 Description : RAII form registrar for automatic form registration.
               Place a static instance in each form's .cpp file:
                 static const gfc::FormRegistrar kReg(WID_FOO, &FFooForm, "Foo");

 Date        : 2026.06.24
*/
//-----------------------------------------------------------------------------
#ifndef GUI_GFC_REGISTRAR_H
#define GUI_GFC_REGISTRAR_H

#include <cstdint>

// Include real type — GWinForm is typedef struct, can't forward-declare
#include "GWinTypes.h"

//-----------------------------------------------------------------------------
namespace gfc {

using FormId = uint16_t;

// Registration function (implemented in GForm.cpp)
void RegisterForm(FormId id, const GWinForm* form, const char* name);

//-----------------------------------------------------------------------------
// RAII registrar: construct one static const instance per form.
// Registration happens before main() via static initialization.
//-----------------------------------------------------------------------------
class FormRegistrar {
public:
    FormRegistrar(FormId id, const GWinForm* form, const char* name) {
        RegisterForm(id, form, name);
    }
};

}  // namespace gfc

//-----------------------------------------------------------------------------
#endif  // GUI_GFC_REGISTRAR_H
