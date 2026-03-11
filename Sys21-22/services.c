#include "calculator.h"
#include <string.h>

Result* add_1_svc(Operands* op, struct svc_req* r){
    static Result result;
    result.Result_u.x = op->a + op->b;
    result.type = 1;
    return(&result);
}
Result* subtract_1_svc(Operands* op, struct svc_req* r){
    static Result result;
    result.Result_u.x = op->a - op->b;
    result.type = 1;
    return(&result);
}
Result* multiply_1_svc(Operands* op, struct svc_req* r){
    static Result result;
    result.Result_u.x = op->a * op->b;
    result.type = 1;
    return(&result);
}
Result* divide_1_svc(Operands* op, struct svc_req* r){
    static Result result;
    if (op->b == 0){
        char * err = "Error, division by 0";
        strcpy(result.Result_u.err, err);
        result.type = 3;
        return (&result);
    }

    result.Result_u.f = (float) op->a / (float) op->b;
    result.type = 2;
    return (&result);

}