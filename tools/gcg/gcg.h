#ifndef __GCG_H__
#define __GCG_H__
#include <glib.h>
#include <gtk/gtktypeutils.h>

extern gboolean in_ident;

typedef const gchar* Id;

#define GET_ID(x) (g_quark_to_string(g_quark_from_string(x)))

typedef const gchar* Header;

typedef struct _Member Member;
typedef struct _PrimType PrimType;
typedef struct _Type Type;
typedef struct _ObjectDef ObjectDef;
typedef struct _Def Def;
typedef struct _EnumDef EnumDef;
typedef struct _FlagsDef FlagsDef;
typedef struct _DataMember DataMember;
typedef struct _Method Method;
typedef struct _MemberClass MemberClass;
typedef struct _DefClass DefClass;
typedef struct _Param Param;
typedef struct _Module Module;


struct _Module {
	Id name;
	Id common_header;
	GHashTable* decl_hash;
};

struct _PrimType {
	Module* module;
	Id name;
	GtkFundamentalType kind;
	Id decl_header;
	Id def_header;
	Def* definition;
};

struct _Type {
	gboolean is_const; /* concerning the _ultimate dereferenced_ object */
	gint indirection; /* num of pointers to type */
	gboolean notnull; /* concerning the _immediate pointer_ */
	PrimType* prim;
};

struct _DefClass {
	void (*output)(Def*);
};

struct _Def {
	PrimType *type;
	GString* doc;
};

#define DEF(x) ((Def*)(x))

extern DefClass enum_class;

struct _EnumDef {
	Def def;
	PrimType* parent;
	GSList* alternatives; /* list of Id */
};

extern DefClass flags_class;

struct _FlagsDef {
	Def def;
	PrimType* parent; /* flags to extend */
	GSList* flags; /* list of Id */
};

extern DefClass object_class;

struct _ObjectDef {
	Def def;
	PrimType* parent;
	Type self_type[2]; /* both non-const and const */
	GSList* members; 
};

typedef enum {
	METH_PUBLIC,
	METH_PROTECTED,
	VIS_PRIVATE
} MethProtection;

typedef enum {
	DATA_READWRITE,
	DATA_READONLY,
	DATA_PROTECTED
} DataProtection;

typedef enum _EmitDef{
	EMIT_NONE,
	EMIT_PRE,
	EMIT_POST
} EmitDef;

typedef enum _MemberType{
	MEMBER_DATA,
	MEMBER_METHOD,
} MemberType;

typedef enum _DataMemberKind{
	DATA_STATIC,
	DATA_DIRECT,
	DATA_STATIC_VIRTUAL
} DataMemberKind;

typedef enum _MethodKind{
	METH_STATIC,
	METH_DIRECT,
	METH_VIRTUAL,
	METH_EMIT_PRE,
	METH_EMIT_POST,
	METH_EMIT_BOTH
} MethodKind;

struct _Member{
	MemberType membertype;
	ObjectDef* my_class;
	Id name;
	GString* doc;
};

#define MEMBER(x) ((Member*)(x))

struct _DataMember {
	Member member;
	DataMemberKind kind;
	DataProtection prot;
	Type type;
};

struct _Method {
	Member member;
	MethodKind kind;
	MethProtection prot;
	GSList* params; /* list of Param* */
	gboolean self_const;
	Type ret_type;
};

struct _Param {
	Id name;
	Method* method;
	GString* doc;
	Type type;
};

typedef void (*DefFunc)(Def* def, gpointer user_data);


void init_db(void);
void put_decl(PrimType* t);
void put_def(Def* d);
PrimType* get_type(Id module, Id type);
Def* get_def(Id module, Id type);
Module* get_mod(Id module);
void foreach_def(DefFunc f, gpointer user_data);




extern Type* type_gtk_type;



#endif
