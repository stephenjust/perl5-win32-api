
/* size_t because it's definitely pointer-size. AFAIK no other int type is on both MSVC and gcc */
typedef union {
	size_t i;
	double d;
	float f;
} stackitem;

void Call_arm64_real(FARPROC, size_t *, double *, stackitem *, unsigned int);
/* GCC requires a union, fnc ptr cast not allowed, error is
   "function called through a non-compatible type" */
typedef union {
    double (*fp) (FARPROC, size_t *, double *, stackitem *, unsigned int);
    long_ptr (*numeric) (FARPROC, size_t *, double *, stackitem *, unsigned int);
} CALL_REAL_U;


enum {
	available_registers = 8
};

void Call_asm(FARPROC ApiFunction, APIPARAM *params, int nparams, APIPARAM *retval)
{
	unsigned int required_stack = 0;

	double float_registers[available_registers] = { 0., 0., 0., 0., 0., 0., 0., 0. };
	size_t int_registers[available_registers] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	stackitem *stack = NULL;

	int i;
    const CALL_REAL_U u_var = {(double (*) (FARPROC, size_t *, double *, stackitem *, unsigned int))Call_arm64_real};

	int used_float_registers = 0;
	int used_int_registers = 0;
	
	// distribute parameters among registers
	for (i = 0; i < nparams; ++i)
	{
		switch (params[i].t+1)
		{
			case T_NUMBER:
			case T_CODE:
			case T_INTEGER:
			case T_CHAR:
			case T_NUMCHAR:
				if (used_int_registers < available_registers)
				{
					int_registers[used_int_registers++] = params[i].l;
				}
				else
				{
					required_stack++;
				}
				break;
			case T_POINTER:
			case T_STRUCTURE:
				if (used_int_registers < available_registers)
				{
					int_registers[used_int_registers++] = (size_t) params[i].p;
				}
				else
				{
					required_stack++;
				}
				break;
			case T_FLOAT: //do not convert the float to a double,
				//put a float in the vX reg, not a double made from a float
				//otherwise a func taking floats will see garbage because
				//vX reg contains a double that is numerically
				//identical/similar to the original float but isn't
				//the original float bit-wise
				if (used_float_registers < available_registers)
				{
					float_registers[used_float_registers++] = *(double *)&(params[i].f);
				}
				else
				{
					required_stack++;
				}
				break;
			case T_DOUBLE:
				if (used_float_registers < available_registers)
				{
					float_registers[used_float_registers++] = params[i].d;
				}
				else
				{
					required_stack++;
				}
				break;
		}
	}
	
#ifdef WIN32_API_DEBUG
	printf("(XS)Win32::API::Call_asm: required_stack=%d, used_float_registers=%d, used_int_registers=%d\n", required_stack, used_float_registers, used_int_registers);
#endif

	if (required_stack)
	{
		stack = _alloca(required_stack * sizeof(*stack));
		memset(stack, 0, required_stack * sizeof(*stack));
		
		// distribute parameters within stack
		int stack_index = 0;
		used_float_registers = 0;
		used_int_registers = 0;

		for (i = 0; i < nparams; ++i)
		{
			switch (params[i].t+1)
			{
				case T_NUMBER:
				case T_CODE:
				case T_INTEGER:
				case T_CHAR:
				case T_NUMCHAR:
					if (used_int_registers++ >= available_registers) {
						stack[stack_index++].i = params[i].l;
					}
					break;
				case T_POINTER:
				case T_STRUCTURE:
					if (used_int_registers++ >= available_registers) {
						stack[stack_index++].i = (size_t) params[i].p;
					}
					break;
				case T_FLOAT:
					if (used_float_registers++ >= available_registers) {
						stack[stack_index++].f = params[i].f;
					}
					break;
				case T_DOUBLE:
					if (used_float_registers++ >= available_registers) {
						stack[stack_index++].d = params[i].d;
					}
					break;
			}
		}
	}

    //use function type punning
	switch (retval->t) {
        //read v0
		case T_FLOAT:
		case T_DOUBLE:
        retval->d = u_var.fp(ApiFunction, int_registers, float_registers, stack, required_stack);
        break;
        //read x0
        default:
        retval->l = u_var.numeric(ApiFunction, int_registers, float_registers, stack, required_stack);
        break;
    }
}


