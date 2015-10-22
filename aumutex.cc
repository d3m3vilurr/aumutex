#include <node.h>
#include <v8.h>
#include <nan.h>

using namespace v8;

#define NanReturn(value) { info.GetReturnValue().Set(value); return; }
#define NanException(type, msg)                                               \
    Exception::type(Nan::New(msg).ToLocalChecked())
#define NanThrowException(exc)                                                \
    {                                                                         \
        Nan::ThrowError(exc); NanReturn(Nan::Undefined());                    \
    }

#define REQUIRE_ARGUMENT_OBJECT(i, var)                                       \
    if (info.Length() <= (i) || !info[i]->IsObject()) {                       \
        NanThrowException(NanException(TypeError,                             \
                                       "Argument " #i " must be a object"));  \
    }                                                                         \
    Local<Object> var = info[i].As<Object>();

#define REQUIRE_ARGUMENT_UTF8_STRING(i, var)                                  \
    if (info.Length() <= (i) || !info[i]->IsString()) {                       \
        NanThrowException(NanException(TypeError,                             \
                                       "Argument " #i " must be a string"));  \
    }                                                                         \
    String::Utf8Value var(info[i]->ToString());

#if defined (_WIN32) || defined (_WIN64)
    #include <windows.h>
    #include <tchar.h>

    #define HANDLE_TYPE HANDLE
    #define CREATE_HANDLE(name, handle, result)                               \
        {                                                                     \
            TCHAR _name[256];                                                 \
            _stprintf(_name, _T("Global\\aumutex_%s"), *name);                \
            handle = CreateMutex(NULL, False, _name);                         \
            result = (handle == NULL);                                        \
        }
    #define LOCK_HANDLE(handle, result)                                       \
        {                                                                     \
            DWORD wait;                                                       \
            wait = WaitForSingleObject(handle, INFINITE);                     \
            result = (wait == WAIT_FAILED)                                    \
        }
    #define UNLOCK_HANDLE(handle, result) result = !ReleaseMutex(handle)
    #define ERROR_NUMBER GetLastError()
    #define CLOSE_HANDLE(handle)                                              \
        {                                                                     \
            ReleaseMutex(handle);                                             \
            CloseHandle(handle);                                              \
        }
#else
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
    #include <stdarg.h>

    #define HANDLE_TYPE long
    #define CREATE_HANDLE(name, handle, result)                               \
        {                                                                     \
            handle = open(*name, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);            \
            result = (handle == -1);                                          \
        }
    #define LOCK_HANDLE(handle, result)                                       \
        {                                                                     \
            struct flock lock;                                                \
            lock.l_type = F_WRLCK;                                            \
            lock.l_start = 0;                                                 \
            lock.l_whence = SEEK_SET;                                         \
            lock.l_len = 0;                                                   \
            result = (fcntl(handle, F_SETLKW, &lock) == -1);                  \
        }
    #define UNLOCK_HANDLE(handle, result)                                     \
        {                                                                     \
            struct flock lock;                                                \
            lock.l_type = F_UNLCK;                                            \
            lock.l_start = 0;                                                 \
            lock.l_whence = SEEK_SET;                                         \
            lock.l_len = 0;                                                   \
            result = (fcntl(handle, F_SETLK, &lock) == -1);                   \
        }
    #define ERROR_NUMBER errno
    #define CLOSE_HANDLE(handle) close(handle);
#endif
NAN_METHOD(create) {
    Nan::HandleScope scope;
    REQUIRE_ARGUMENT_UTF8_STRING(0, param_name);
    HANDLE_TYPE mutex;
    bool failure;
    CREATE_HANDLE(param_name, mutex, failure);
    if (failure) {
        char msg[256];
        snprintf(msg, 255,
                 "create handle(%s) error:%d", *param_name, ERROR_NUMBER);
        NanThrowException(NanException(TypeError, msg));
    }
    Local<ObjectTemplate> templ = ObjectTemplate::New();
    templ->SetInternalFieldCount(1);

    Local<Object> inst = templ->NewInstance();
    inst->SetInternalField(0, Nan::New<External>((void*)mutex));
    NanReturn(inst);
}

NAN_METHOD(enter) {
    Nan::HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Local<External> field = wrapper->GetInternalField(0).As<External>();

    bool failure;
    LOCK_HANDLE((HANDLE_TYPE)(field->Value()), failure);
    if (failure) {
        char msg[256];
        snprintf(msg, 255, "Lock error:%d", ERROR_NUMBER);
        NanThrowException(NanException(TypeError, msg));
    }
    NanReturn(Nan::Undefined());
}

NAN_METHOD(leave) {
    Nan::HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Local<External> field = wrapper->GetInternalField(0).As<External>();

    bool failure;
    UNLOCK_HANDLE((HANDLE_TYPE)(field->Value()), failure);
    if (failure) {
        char msg[256];
        snprintf(msg, 255, "unlock error:%d", ERROR_NUMBER);
        NanThrowException(NanException(TypeError, msg));
    }
    NanReturn(Nan::Undefined());
}

NAN_METHOD(close) {
    Nan::HandleScope scope;
    REQUIRE_ARGUMENT_OBJECT(0, wrapper);
    Local<External> field = wrapper->GetInternalField(0).As<External>();

    CLOSE_HANDLE((HANDLE_TYPE)(field->Value()));

    NanReturn(Nan::Undefined());
}

NAN_MODULE_INIT(init) {
    target->Set(Nan::New("create").ToLocalChecked(),
                Nan::New<FunctionTemplate>(create)->GetFunction());
    target->Set(Nan::New("enter").ToLocalChecked(),
                Nan::New<FunctionTemplate>(enter)->GetFunction());
    target->Set(Nan::New("leave").ToLocalChecked(),
                Nan::New<FunctionTemplate>(leave)->GetFunction());
    target->Set(Nan::New("close").ToLocalChecked(),
                Nan::New<FunctionTemplate>(close)->GetFunction());
}

NODE_MODULE(aumutex, init)
