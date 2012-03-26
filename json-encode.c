/*
 * [json-encodes] encodes data as JSON and outputs it as a symbol.
 * */

#include "purest_json.h"

static t_class *json_encode_class;

void json_encode_free_memory(t_json_encode *x) {
	t_key_value_pair *data_to_free;
	t_key_value_pair *next_data;

	data_to_free = x->data;
	while(data_to_free != NULL) {
		next_data = data_to_free->next;
		freebytes(data_to_free->key, MAXPDSTRING);
		freebytes(data_to_free->value, MAXPDSTRING);
		freebytes(data_to_free, sizeof(t_key_value_pair));
		data_to_free = next_data;
	}

	x->data_count = 0;
	x->data = NULL;
}

static json_object *create_object(char *value) {
	json_object *object;
	char *parsed_string;
	size_t memsize = 0;
	/* if stored value is string is starting with { and ending with }, 
	   then create a json object from it. */
	if (value[0] == '{' && value[strlen(value) - 1] == '}') {
		parsed_string = remove_backslashes(value, memsize);;
		object = json_tokener_parse(parsed_string);
		freebytes(parsed_string, memsize);
	} else {
		object = json_object_new_string(value);
	}
	return object;
}

void setup_json0x2dencode(void) {
	json_encode_class = class_new(gensym("json-encode"), (t_newmethod)json_encode_new,
			(t_method)json_encode_free, sizeof(t_json_encode), 0, A_GIMME, 0);
	class_addbang(json_encode_class, (t_method)json_encode_bang);
	class_addmethod(json_encode_class, (t_method)json_encode_add, gensym("add"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_array_add, gensym("array"), A_GIMME, 0);
	class_addmethod(json_encode_class, (t_method)json_encode_clear, gensym("clear"), A_GIMME, 0);
	class_sethelpsymbol(json_encode_class, gensym("json"));
}

void *json_encode_new(t_symbol *selector, int argcount, t_atom *argvec) {
	t_json_encode *x = (t_json_encode *)pd_new(json_encode_class);

	(void) selector;
	(void) argcount;
	(void) argvec;

	x->data_count = 0;
	outlet_new(&x->x_ob, NULL);
	return (void *)x;
}

void json_encode_free (t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	json_encode_free_memory(x);
}

void json_encode_bang(t_json_encode *x) {
	int i, j, k; 
	int array_member_numbers[x->data_count];
	int array_member_count = 0;
	short already_added = 0;
	t_key_value_pair *data_member;
	t_key_value_pair *data_member_compare;
	json_object *jobj = json_object_new_object();
	json_object *value;
	json_object *array_members[x->data_count];

	if (x->data_count > 0) {
		data_member = x->data;
		for (i = 0; i < x->data_count; i++) {
			already_added = 0;
			/* Is it an array member? */
			if (data_member->is_array == 1) {
				value = json_object_new_array();
				data_member_compare = data_member;
				for (j = i; j < x->data_count; j++) {
					if (strcmp(data_member_compare->key, data_member->key) == 0) {
						for (k = 0; k < array_member_count; k++) {
							/* If already inserted, continue i loop */
							if (array_member_numbers[k] == j) {
								already_added = 1;
								break;
							}
						}
						if (already_added == 0) {
							json_object *array_member = create_object(data_member_compare->value);
							json_object_array_add(value, array_member);
							array_member_numbers[array_member_count] = j;
							array_members[array_member_count] = array_member;
							array_member_count++;
						}
					}
					data_member_compare = data_member_compare->next;
				}
			} else {
				value = create_object(data_member->value);
			}
			if (already_added == 0) {
				json_object_object_add(jobj, data_member->key, value); 
			}
			data_member = data_member->next;
		}
		outlet_symbol(x->x_ob.ob_outlet, gensym(json_object_to_json_string(jobj)));
		for (i = 0; i < array_member_count; i++) {
			json_object_put(array_members[i]);
		}
		json_object_put(jobj);
	}
}

void json_encode_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *key;
	char *value;
	char temp_value[MAXPDSTRING];
	t_key_value_pair *created_data = NULL;
	t_key_value_pair *previous_data = NULL;
	int i;

	(void) selector;

	if (argcount < 2) {
		error("For method 'add' You need to specify a value.");
	} else {
		created_data = (t_key_value_pair *)getbytes(sizeof(t_key_value_pair));
		key = (char *)getbytes(MAXPDSTRING * sizeof(char));
		value = (char *)getbytes(MAXPDSTRING * sizeof(char));
		if (created_data == NULL || key == NULL || value == NULL) {
			error("Could not allocate memory.");
			return;
		}
		atom_string(argvec, key, MAXPDSTRING);
		created_data->key = key;
		atom_string(argvec + 1, value, MAXPDSTRING);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
		created_data->value = value;
		created_data->next = NULL;
		created_data->is_array = 0;
		if (x->data == NULL) {
			x->data = created_data;
		} else {
			previous_data = x->data;
			while (previous_data->next) {
				previous_data = previous_data->next;
			}
			previous_data->next = created_data;
		}

		x->data_count++;
	}
}

void json_encode_array_add(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	char *key;
	char *value;
	char temp_value[MAXPDSTRING];
	t_key_value_pair *created_data = NULL;
	t_key_value_pair *previous_data = NULL;
	int i;

	(void) selector;

	if (argcount < 2) {
		error("For method 'array' You need to specify a value.");
	} else {
		created_data = (t_key_value_pair *)getbytes(sizeof(t_key_value_pair));
		key = (char *)getbytes(MAXPDSTRING * sizeof(char));
		value = (char *)getbytes(MAXPDSTRING * sizeof(char));
		if (created_data == NULL || key == NULL || value == NULL) {
			error("Could not get data");
			return;
		}
		atom_string(argvec, key, MAXPDSTRING);
		created_data->key = key;
		atom_string(argvec + 1, value, MAXPDSTRING);
		for(i = 2; i < argcount; i++) {
			atom_string(argvec + i, temp_value, MAXPDSTRING);
			strcat(value, " ");
			strcat(value, temp_value);
		}
		created_data->value = value;
		created_data->next = NULL;
		created_data->is_array = 1;
		if (x->data == NULL) {
			x->data = created_data;
		} else {
			previous_data = x->data;
			while (previous_data->next) {
				previous_data = previous_data->next;
			}
			previous_data->next = created_data;
		}
		x->data_count++;
	}
}

void json_encode_clear(t_json_encode *x, t_symbol *selector, int argcount, t_atom *argvec) {
	(void) selector;
	(void) argcount;
	(void) argvec;

	json_encode_free_memory(x);
}
