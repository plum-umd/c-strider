 
(* xfgen.ml
 * 
 * This module implements the generation of transformation code based
 * on the results of the comparison implemented in kttcompare.ml.  
 *)

open Kttools

open Kttypes.TransformTypes
open Kttypes.Tools
open Kttcompare

open Xftypes

module H = Hashtbl
module L = List
module S = String

(************************* DEBUG PRINTS **********************************)
let dbg =  (*false*) true
let dbg_print s =
   if dbg then print_endline s
(*************************************************************************)

(* The following types and functions maintain the current contextual
   information that is passed through the code as comparisons are
   occuring. *)

type elem_key = 
  | KeyTd of string
  | KeyFn of string
  | KeyO of string
let elem_key_to_s = function
  | KeyTd s -> s
  | KeyFn s -> s
  | KeyO s -> s

type gencontext = {
  cur_elem : elem_key option;
  cur_renamer : (string -> string) option;
  cur_xform : string ref;
  print_context : string;
  soft_deps : (elem_key, elem_key) H.t;
  hard_deps : (elem_key, elem_key) H.t;
  rendered : (elem_key, string) H.t;
  prototypes : (elem_key, string) H.t;
  xform_funs : elem_key list ref;
  xform_generics : (elem_key, string list) H.t;
  td_incomplete : (elem_key, typ) H.t;
  code : string ref;
  externs_to_reg : string list ref;
  visitall : string list ref;
  macrotypes : string list ref;
  macrotypes_generics_maintype : string list ref;
  macrotypes_target_type : string list ref;
  macrotypes_ptrtome_type : string list ref;
  macrotypes_funs : string list ref;
  stored_num_gen_args : string list ref; 
  stored_gen_args : string list ref; 
  sizes : string list ref;
  prims : string list ref;
  arrlens : string list ref;
  arrs : string list ref;
}

let gencontext_create () = {
  cur_elem = None;
  cur_renamer = None;
  cur_xform = ref "";
  print_context = "";
  soft_deps = H.create 37;
  hard_deps = H.create 37;
  rendered = H.create 37;
  prototypes = H.create 37;
  xform_funs = ref [];
  xform_generics = H.create 37;
  td_incomplete = H.create 37;
  code = ref "";
  externs_to_reg = ref [];
  visitall = ref [];
  arrs = ref [];
  macrotypes = ref (
   "TYPE_CHAR, " ::
   "TYPE_SIGNED_CHAR, " ::
   "TYPE_UNSIGNED_CHAR, " ::
   "TYPE_CHAR_NT, " ::
   "TYPE_BOOL, " ::
   "TYPE_INT, " ::
   "TYPE_UNSIGNED_INT, " ::
   "TYPE_SHORT, " ::
   "TYPE_UNSIGNED_SHORT, " ::
   "TYPE_LONG, " ::
   "TYPE_UNSIGNED_LONG, " ::
   "TYPE_LONG_LONG, " ::
   "TYPE_UNSIGNED_LONG_LONG, " ::
   "TYPE_FLOAT, " ::
   "TYPE_DOUBLE, " ::
   "TYPE_LONG_DOUBLE, " ::
   "TYPE_FUNPTR, " ::
   "TYPE_OPAQUE_PTR, " :: (* for [opaque] generics *)
   []);
  macrotypes_generics_maintype = ref (
   "TYPE_CHAR, " ::
   "TYPE_SIGNED_CHAR, " ::
   "TYPE_UNSIGNED_CHAR, " ::
   "TYPE_CHAR_NT, " ::
   "TYPE_BOOL, " ::
   "TYPE_INT, " ::
   "TYPE_UNSIGNED_INT, " ::
   "TYPE_SHORT, " ::
   "TYPE_UNSIGNED_SHORT, " ::
   "TYPE_LONG, " ::
   "TYPE_UNSIGNED_LONG, " ::
   "TYPE_LONG_LONG, " ::
   "TYPE_UNSIGNED_LONG_LONG, " ::
   "TYPE_FLOAT, " ::
   "TYPE_DOUBLE, " ::
   "TYPE_LONG_DOUBLE, " ::
   "TYPE_FUNPTR, " ::
   "TYPE_OPAQUE_PTR, " :: (* for [opaque] generics *)
   []);
   stored_num_gen_args=  ref [];
   stored_gen_args=  ref [];
   arrlens = ref (
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " ::
   "0, " :: (* for [opaque] generics *)
   []);
  macrotypes_target_type = ref (
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " :: (* for [opaque] generics *)
   []);
   macrotypes_ptrtome_type = ref (
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " ::
   "-1, " :: (* for [opaque] generics *)
   []);
  macrotypes_funs = ref (
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " ::
   "transform_prim, " :: (* for [opaque] generics *)
   []);
  sizes = ref (
   "sizeof(char), " ::
   "sizeof(signed char), " ::
   "sizeof(unsigned char), " ::
   (*"0, " ::*) "sizeof(char*), " ::
   "sizeof(_Bool), " ::
   "sizeof(int), " ::
   "sizeof(unsigned int), " ::
   "sizeof(short), " ::
   "sizeof(unsigned short), " ::
   "sizeof(long), " ::
   "sizeof(unsigned long), " ::
   "sizeof(long long), " ::
   "sizeof(unsigned long long), " ::
   "sizeof(float), " ::
   "sizeof(double), " ::
   "sizeof(long double), " ::
   "sizeof(void *), " ::
   "sizeof(void *), " :: 
   []);
  prims = ref [];
}


(*get rid of special characters from enums*)
let rec parseit str c : string =
  try
    (let i = String.index str c in
    (String.set str i '_'); parseit str c )
  with Not_found ->
    str

let compute_gen_arg_var_name gen_arg_name = "gen_" ^ gen_arg_name;;

(*TODO, now that i know what i need, make this consolidated into 2 fun*)
let scrub str : string =
  (parseit (parseit (parseit (parseit str '@') '#') '/') '.')


(*TODO, I'm sure these global vars are an ocaml abomination, but I am short on time and don't want to change all params now*)
let current_name = ref ""
let current_name_field = ref ""
let current_dyn_arr_upd = ref ""
let gen_decls = ref ""

let gencontext_append_symbol (gen_ctx:gencontext) (s0:string) (s1:string) = 
  let append_str = if s0 = s1 then s0 else s0 ^ " -> " ^ s1 in
  { 
    gen_ctx with print_context = 
      if gen_ctx.print_context = "" then append_str
      else gen_ctx.print_context ^ "." ^ append_str
  }

let gencontext_get_symbol (gen_ctx:gencontext) = gen_ctx.print_context

let genError gen_ctx (s:string) =
  exitError (gencontext_get_symbol gen_ctx ^ ": " ^ s)

let genWarning gen_ctx (s:string) =
  print_endline (gencontext_get_symbol gen_ctx ^ ": " ^ s)

let gencontext_add_dep (gen_ctx:gencontext) (is_hard:bool) (d:elem_key) =
  match gen_ctx.cur_elem with
    | Some f -> 
      let dep_hash = if is_hard then gen_ctx.hard_deps else gen_ctx.soft_deps in
      if f <> d && not (let deps = H.find_all dep_hash f in List.mem d deps) then
	begin 
	  (*let s = elem_key_to_s d in
	  print_endline ("Adding dependency "^s^" to curelem "^(elem_key_to_s f)); *)
          H.add dep_hash f d
	end 
    | None -> genError gen_ctx ("gencontext_add_hard_dep called with no current item " ^ elem_key_to_s d)

let gencontext_mark_incomplete (gen_ctx:gencontext) (t:typ) =
  match gen_ctx.cur_elem with
    | Some ((KeyTd td_name) as c) -> 
      if not (H.mem gen_ctx.td_incomplete c) then
        begin
	  if not ((String.sub td_name 19 2) = "__") then (* filter gcc builtins *)
            print_endline (td_name ^ " is incomplete");
          H.add gen_ctx.td_incomplete c t
        end
    | Some _ ->
      genError gen_ctx ("Did not expect to find incomplete type during processing: " ^ typeToString t)
    | None -> 
      failwith "gencontext_mark_incomplete called with no current item"

let gencontext_add_render (gen_ctx:gencontext) (k:elem_key) (rendered:string) =
  if H.mem gen_ctx.rendered k then
    (if rendered <> H.find gen_ctx.rendered k then
        genWarning gen_ctx ("Multiple conflicting definitions for :" ^ elem_key_to_s k))
  else
    H.add gen_ctx.rendered k rendered

let gencontext_add_prototype (gen_ctx:gencontext) (k:elem_key) (proto:string) =
  if H.mem gen_ctx.prototypes k then
    assert (proto = H.find gen_ctx.prototypes k)
  else
    H.add gen_ctx.prototypes k proto

let gencontext_set_current (gen_ctx:gencontext) (current:elem_key) = 
  { gen_ctx with cur_elem = Some current }

let gencontext_set_renamer (gen_ctx:gencontext) (renamer:string->string) = 
  { gen_ctx with cur_renamer = Some renamer }

let gencontext_set_xform (gen_ctx:gencontext) (fname:string) = 
   gen_ctx.cur_xform := fname 

let gencontext_rename (gen_ctx:gencontext) (name:string) = 
  match gen_ctx.cur_renamer with
    | Some r -> r name
    | None -> name (*now used for making ubertypes *)(*failwith "gencontext_rename called with no current renamer"*)

let gencontext_add_xformer (gen_ctx:gencontext) (k:elem_key) (proto:string) (rendered:string) =
  gencontext_add_prototype gen_ctx k proto;
  gencontext_add_render gen_ctx k rendered;
  gen_ctx.xform_funs := k :: !(gen_ctx.xform_funs)

let gencontext_set_xform_generics gen_ctx xform_name (gen_args0, gen_args1) =
  let generics = (L.map (fun (GenName n) -> n) gen_args0) in
  if (H.mem gen_ctx.xform_generics xform_name) then
    begin
      let prev_generics = H.find gen_ctx.xform_generics xform_name in
      assert (generics = prev_generics);
    end;
  assert (gen_args0 = gen_args1);
  H.add gen_ctx.xform_generics xform_name generics

let gencontext_get_xform_generics gen_ctx xform_name =
  if (H.mem gen_ctx.xform_generics xform_name)
     then (H.find gen_ctx.xform_generics xform_name)
     else [] 

(* These renamers are used to ensure that the type information that we
   output to the generated file will not collide with other
   definitions from the original program that are #included in the
   global C block. *)
let old_rename (name:string) =
  "_kitsune_old_rename_" ^ name

let new_rename (name:string) =
  "_kitsune_new_rename_" ^ name

let in_MACRO_rename (name:string) =
  name 

let out_MACRO_rename (name:string) =
  name ^ "_OUT" 

let typeenum_ikindSizeToString = function
  | IChar -> "char"
  | ISChar ->"signed char"
  | IUChar ->"unsigned char"
  | IBool -> "_Bool"
  | IInt ->  "int"
  | IUInt -> "unsigned int"
  | IShort ->"short"
  | IUShort ->"unsigned short"
  | ILong -> "long"
  | IULong -> "unsigned long"
  | ILongLong ->"long long"
  | IULongLong -> "unsigned long long"

let typeenum_fkindSizeToString = function
  | FFloat -> "float"
  | FDouble ->"double"
  | FLongDouble ->"long double"

let typeenum_ikindToString = function
  | IChar -> "TYPE_CHAR"
  | ISChar -> "TYPE_SIGNED_CHAR"
  | IUChar -> "TYPE_UNSIGNED_CHAR"
  | IBool -> "TYPE_BOOL"
  | IInt -> "TYPE_INT"
  | IUInt -> "TYPE_UNSIGNED_INT"
  | IShort -> "TYPE_SHORT"
  | IUShort -> "TYPE_UNSIGNED_SHORT"
  | ILong -> "TYPE_LONG"
  | IULong -> "TYPE_UNSIGNED_LONG"
  | ILongLong -> "TYPE_LONG_LONG"
  | IULongLong -> "TYPE_UNSIGNED_LONG_LONG"

let typeenum_fkindToString = function
  | FFloat -> "TYPE_FLOAT"
  | FDouble -> "TYPE_DOUBLE"
  | FLongDouble -> "TYPE_LONG_DOUBLE"


(* TODO, i need to think about this a bit more *)
(*let typeenum_typeToString = function
  | TVoid (Some (GenName name)) -> "void<" ^ name ^ ">"
  | TVoid None -> "void"
  | TInt ik -> ikindToString ik
  | TFloat fk -> fkindToString fk
  | TPtr (t', None, mem) -> "ptr_single" ^ (mallocToString mem)  ^ "(" ^ (typeToString t') ^ ")"
  | TPtr (t', Some (GenName name), mem) -> typeToString t' ^ "<@" ^ name ^ ">" ^ (mallocToString mem)
  | TPtrOpaque t' -> "ptr_opaque" ^ "(" ^ (typeToString t') ^ ")"
  | TPtrArray (t', len, mem) -> "ptr_array" ^ (lenToString len) ^ (mallocToString mem) ^ "(" ^ (typeToString t') ^ ")"
  | TArray (t', len) -> "array" ^ (lenToString len) ^ "(" ^ (typeToString t') ^ ")"
  | TFun (rt, args, varargs) -> "funptr"
  | TNamed (tdname, gen_in) -> tdname ^ (genInToString gen_in)
  | TStruct (sname, gen_in) -> "struct " ^ sname ^ (genInToString gen_in)
  | TUnion (uname, gen_in) -> "union " ^ uname ^ (genInToString gen_in)
  | TEnum ename -> "enum " ^ ename
  | TBuiltin_va_list -> "var_args"
*)

let populate_type_info gen_ctx (macrot_in:string) (maint:string) (targett:string) (ptrto:string)
   (arrlen:string) (sizestr:string) funstr :unit =
   gen_ctx.macrotypes := L.append !(gen_ctx.macrotypes) [macrot_in^ ", "];
   gen_ctx.macrotypes_generics_maintype := L.append !(gen_ctx.macrotypes_generics_maintype) [maint^ ", "];(*TODO adding "OUT" here breaks ints, etc*)
   gen_ctx.macrotypes_target_type := L.append !(gen_ctx.macrotypes_target_type) [targett^ ", "]; 
   gen_ctx.macrotypes_ptrtome_type := L.append !(gen_ctx.macrotypes_ptrtome_type) [ptrto^ ", "]; 
   gen_ctx.arrlens := L.append !(gen_ctx.arrlens) [arrlen^ ", "];
   gen_ctx.sizes := L.append !(gen_ctx.sizes) [sizestr^ ", "];
   gen_ctx.macrotypes_funs := L.append !(gen_ctx.macrotypes_funs) [funstr^ ", " ] 



let rec append_deref gen_ctx (s:string) (d:int) : string =
   
   let found = try (Str.search_forward  (Str.regexp "->") s 0) with Not_found -> 0 in
   if (found!=0) then s else
   
   match d with
   | 0 -> s
   | _ -> 
     begin
        if not (L.exists (fun x -> x = (s^"_PTR" ^", " )) !(gen_ctx.macrotypes) ) then
          (*TODO adding "OUT" to targettypes breaks ints, etc*)
          ( populate_type_info gen_ctx (s^"_PTR")  (s^"_PTR") ((scrub s)) ("0") ("-1") ("sizeof (void * )") (!(gen_ctx.cur_xform));
          (*TODO make "void *" point to the actual type, but here the size will always be the same *)
           populate_type_info gen_ctx (s^"_OUT_PTR") (s^"_PTR") ((scrub s)) (*((scrub s)^"_OUT")*) ("0") ("-1") ("sizeof (void * )") (!(gen_ctx.cur_xform) ) );
        append_deref gen_ctx (s^"_PTR") (d-1)
     end 


(*TODO replace the below AddSizeOf with this *)
let rec render_sizeof gen_ctx  (t:typ) (renamer:string)  (i:int) = 
  let star = match i with
    | 0 -> ""
    | _ -> "*" (*TODO this is pretty janky, but so are generics, so...*)
  in
  match t with
    | TInt ik -> "sizeof(int)"
    | TFloat fk -> "sizeof(float)"
    | TPtrOpaque pt  
    | TPtrArray (pt, _, _) 
    | TPtr (pt, _, _) -> "sizeof(void*)"
    | TArray (t0', len0)-> (render_sizeof gen_ctx  t0' renamer i)  (*only want size of the base elem*)
    | TFun (rt, args, is_varargs) -> "sizeof(void* )"
    | TNamed (nm, _) ->  "sizeof(" ^ renamer ^ nm ^ star ^ ")" 
    | TStruct (nm, _) -> "sizeof(struct "^ renamer ^ nm ^ star ^ ")" 
    | TUnion (nm, _) ->  "sizeof(union " ^ renamer ^ nm ^ star ^ ")"
    | TEnum nm ->        "sizeof(enum "  ^ renamer ^ nm ^ star ^ ")"
    | _ -> "sizeof(void*)" (*TODO*)


type what_to_render = 
  | AddMacro
  | AddDep
  | AddSizeOf
  | MacroOnly

(* The following code implements rendering program elements (only the
   type definitions) into the generated file.   
    AddMacro   Creates macroized name "STRUCT_" (and adds to gencontext.macrotypes)
    AddDep    Adds dependency to gencontext_add_dep (this was the original use of this fun)
    AddSizeOf    Does "sizeof" (and adds to gencontext.sizes) 
    MacroOnly creates macroized name ONLY.  don't add to ANYTHING.
   (*TODO*) this is sort of chaotic, but working. 

   btw, genarg_ptr necessary becaues of annoying generate_genin_xform_fullinner_type ptr problem with gd0...
*)
let rec render_type gen_ctx (t:typ) (under_ptr:bool) (genarg_ptr:bool) (nameopt:string option) (add:what_to_render) = 

  let pname = match nameopt with
      Some v -> v | None -> ""
  in        
  match t with
    | TVoid n -> (match add with
        | MacroOnly -> ( 
                match t with 
                | TVoid (Some (GenName gv0)) -> 
                  (compute_gen_arg_var_name gv0)^"->type"
                |_ -> if not(!(gen_ctx.cur_xform) = "transform_fptr") then
                      begin
                          if ((!current_name_field) = "" ) then
                            dbg_print ("!!!!!!!!!!! WARNING: Found void*: \n" ^ (!(current_name)))
                          else
                            dbg_print ("!!!!!!!!!!! WARNING: Found void*: "^ (!(current_name_field)) ^ "inside of \n" ^ (!(current_name)))
                      end;
                     "TYPE_OPAQUE_PTR"
               )
        | _ ->  (if (not under_ptr) then gencontext_mark_incomplete gen_ctx t;
       "void " ^ pname))
    | TInt ik -> (match add with
        | MacroOnly -> 
            (match ik, genarg_ptr with
              | IChar,  true
              | ISChar, true
              | IUChar, true ->let _ = (append_deref gen_ctx "TYPE_CHAR_NT"  1) in (); "TYPE_CHAR_NT"
              | _ -> typeenum_ikindToString ik)
        | _ -> (ikindToString ik) ^ " " ^ pname)
    | TFloat fk -> (match add with
        | MacroOnly -> typeenum_fkindToString fk
        | _ -> (fkindToString fk) ^ " " ^ pname)
    | TPtrOpaque pt -> (match add with
        | AddDep -> render_type gen_ctx pt true false (Some ("(*" ^ pname ^ ")")) add 
        | MacroOnly -> "TYPE_OPAQUE_PTR"
        | _ -> render_type gen_ctx pt true false nameopt add)
    | TPtr (pt, _, _) -> (match add with
        | AddDep -> render_type gen_ctx pt true false (Some ("(*" ^ pname ^ ")")) add
        | MacroOnly -> (match pt with 
                  (* TODO THIS | TPtr (_, Some (GenName gv0), _) *)
                  | TInt i -> (match i with
                    | IChar
                    | ISChar
                    | IUChar ->let _ = (append_deref gen_ctx "TYPE_CHAR_NT"  1) in (); "TYPE_CHAR_NT"
                    | _ -> render_type gen_ctx pt true false nameopt add )
                  | _ -> render_type gen_ctx pt true false  nameopt add )
        | _ -> render_type gen_ctx pt true false  nameopt add)
    | TPtrArray (pt, _, _) -> (match add with
        | AddDep -> render_type gen_ctx pt true false (Some ("(*" ^ pname ^ ")")) add
        | MacroOnly -> (match pt with 
                  | TInt i -> (match i with
                    | IChar
                    | ISChar
                    | IUChar -> "TYPE_CHAR" (*:if it's declared as an array...then it's not _NT.  TODO dbl check*)
                    | _ -> render_type gen_ctx pt true false nameopt add )
                  | _ -> render_type gen_ctx pt true false nameopt add )
        | _ -> render_type gen_ctx pt true false nameopt add)
    | TArray (t', Len_Int lv) ->  (match add with
        | AddDep -> render_type gen_ctx t' under_ptr false (Some (pname ^ "[" ^ (Int64.to_string lv) ^ "]")) add
        | _ -> render_type gen_ctx t' under_ptr false nameopt add)
    | TArray (t', _) -> (match add with
        | AddDep -> render_type gen_ctx t' under_ptr false (Some (pname ^ "[]")) add
        | _ -> render_type gen_ctx t' under_ptr false nameopt add)
        
    | TFun (rt, args, is_varargs) ->
      if ((not under_ptr) && (add !=MacroOnly)) then
        gencontext_mark_incomplete gen_ctx t;
      
      let writeArg (aname, atype) =
        render_type gen_ctx atype true false (Some aname) add
      in
      let arg_str_list = 
        if is_varargs then
          (L.map writeArg args) @ ["..."]
        else
          (L.map writeArg args)
      in
      (match add with
        | AddDep -> (let arg_str = S.concat ", " arg_str_list in
          render_type gen_ctx rt true false (Some (pname ^ "(" ^ arg_str ^ ")")) add)
        | MacroOnly -> "TYPE_FUNPTR"
        | _ -> render_type gen_ctx rt true false nameopt add)

    | TNamed (nm, _) ->
      let td_name = gencontext_rename gen_ctx nm in
      (match add with
        | AddDep -> (gencontext_add_dep gen_ctx (not under_ptr) (KeyTd td_name);
          td_name ^ " " ^ pname)
        | AddMacro -> (gen_ctx.macrotypes := L.append !(gen_ctx.macrotypes) ["TYPE_" ^(scrub td_name)^ ", "];
               gen_ctx.macrotypes_generics_maintype := L.append !(gen_ctx.macrotypes_generics_maintype) ["TYPE_" ^(scrub td_name)^ ", "];
               gen_ctx.macrotypes_target_type := L.append !(gen_ctx.macrotypes_target_type) ["-1, "];
               gen_ctx.macrotypes_ptrtome_type := L.append !(gen_ctx.macrotypes_ptrtome_type) ["-1, "];
               gen_ctx.arrlens := L.append !(gen_ctx.arrlens) ["0, "];
          "TYPE_" ^ td_name ^ " " ^ pname)
        | AddSizeOf -> (let struct_name = "sizeof(" ^ (gencontext_rename gen_ctx nm) ^ "), " in
          gen_ctx.sizes := L.append  !(gen_ctx.sizes) [struct_name];
          struct_name ^ " " ^ pname;)
        | MacroOnly ->  ("TYPE_" ^ (S.trim td_name)))
    | TStruct (nm, _) ->
      (match add with
        | AddDep ->  (let struct_name = "struct " ^ (gencontext_rename gen_ctx nm) in
          gencontext_add_dep gen_ctx (not under_ptr) (KeyO struct_name); 
          struct_name ^ " " ^ pname)
        | AddMacro ->  (let struct_name = "TYPE_STRUCT_" ^ (gencontext_rename gen_ctx nm) in
          gen_ctx.macrotypes := L.append !(gen_ctx.macrotypes) [(scrub struct_name) ^ ", "] ;
          gen_ctx.macrotypes_generics_maintype := L.append !(gen_ctx.macrotypes_generics_maintype) [(scrub struct_name) ^ ", "] ;
          gen_ctx.macrotypes_target_type := L.append !(gen_ctx.macrotypes_target_type) ["-1, "] ;
          gen_ctx.macrotypes_ptrtome_type := L.append !(gen_ctx.macrotypes_ptrtome_type) ["-1, "] ;
          gen_ctx.arrlens := L.append !(gen_ctx.arrlens) ["0, "];
          struct_name ^ " " ^ pname)
        | AddSizeOf -> (let struct_name = "sizeof(struct " ^ (gencontext_rename gen_ctx nm) ^ "), " in
          gen_ctx.sizes :=  L.append !(gen_ctx.sizes) [struct_name];
          struct_name ^ " " ^ pname)
        | MacroOnly ->  ("TYPE_STRUCT_" ^ (gencontext_rename gen_ctx nm)))
    | TUnion (nm, _) ->
      (match add with
        | AddDep -> (let union_name = "union " ^ (gencontext_rename gen_ctx nm) in
          gencontext_add_dep gen_ctx (not under_ptr) (KeyO union_name);
          union_name ^ " " ^  pname)
        | AddMacro -> (let union_name = "TYPE_UNION_" ^ (gencontext_rename gen_ctx nm) in
          gen_ctx.macrotypes :=  L.append !(gen_ctx.macrotypes) [(scrub union_name) ^ ", "];
          gen_ctx.macrotypes_generics_maintype :=  L.append !(gen_ctx.macrotypes_generics_maintype) [(scrub union_name) ^ ", "];
          gen_ctx.macrotypes_target_type :=  L.append !(gen_ctx.macrotypes_target_type) ["-1, "];
          gen_ctx.macrotypes_ptrtome_type :=  L.append !(gen_ctx.macrotypes_ptrtome_type) ["-1, "];
          gen_ctx.arrlens := L.append !(gen_ctx.arrlens) ["0, "];
          Format.printf "!!!!!!!!!!! WARNING: Found union: %s\n" (union_name ^ " " ^  pname);
          union_name ^ " " ^  pname)
        | AddSizeOf -> (let union_name = "sizeof(union " ^ (gencontext_rename gen_ctx nm) ^ "), " in
          gen_ctx.sizes := L.append !(gen_ctx.sizes) [union_name];
          union_name ^ " " ^ pname)
        | MacroOnly -> ("TYPE_UNION_" ^ (gencontext_rename gen_ctx nm)))
    | TEnum nm ->
      (match add with
        | AddDep -> (let enum_name = "enum " ^ (gencontext_rename gen_ctx nm) in
          gencontext_add_dep gen_ctx (not under_ptr) (KeyO enum_name);
          enum_name ^ " " ^ pname)
        | AddMacro -> (let enum_name = "TYPE_ENUM_" ^ (gencontext_rename gen_ctx nm) in
          gen_ctx.macrotypes := L.append !(gen_ctx.macrotypes) [(scrub enum_name) ^ ", "];
          gen_ctx.macrotypes_generics_maintype := L.append !(gen_ctx.macrotypes_generics_maintype) [(scrub enum_name) ^ ", "];
          gen_ctx.macrotypes_target_type := L.append !(gen_ctx.macrotypes_target_type) [ "-1, "];
          gen_ctx.macrotypes_ptrtome_type := L.append !(gen_ctx.macrotypes_ptrtome_type) [ "-1, "];
          gen_ctx.arrlens := L.append !(gen_ctx.arrlens) ["0, "];
          enum_name ^ " " ^ pname)
        | AddSizeOf -> (let enum_name = "sizeof(enum " ^ (gencontext_rename gen_ctx nm) ^ "), " in
          gen_ctx.sizes := L.append !(gen_ctx.sizes) [enum_name];
          enum_name ^ " " ^ pname)
        | MacroOnly -> ("TYPE_INT")) (*this is the VALUE of the enum itself, so just a number *)

    | TBuiltin_va_list -> 
      "__builtin_va_list " ^ pname

let render_field gen_ctx (finfo:field) =
  let bitfield_string = 
    match finfo.fbitfield with
      | None -> ""
      | Some sz -> ":" ^ (string_of_int sz)
  in
  (render_type gen_ctx finfo.ftype false false (Some finfo.fname) AddDep) ^ bitfield_string ^ ";\n"

let handle_typedef gen_ctx (tinfo:typedef_info) =  
  let td_name = gencontext_rename gen_ctx tinfo.tname in
  (* let () = print_endline ("Handling typedef "^tinfo.tname^" (curelem "^td_name^")") in *)
  let gen_ctx' = gencontext_set_current gen_ctx (KeyTd td_name) in
  gencontext_add_render gen_ctx' (KeyTd td_name)
    ("typedef " ^
        (render_type gen_ctx' tinfo.ttype false false (Some (gencontext_rename gen_ctx' tinfo.tname)) AddDep) ^
        ";\n")

let handle_struct gen_ctx (sinfo:struct_info) =
  let struct_name = "struct " ^ (gencontext_rename gen_ctx sinfo.sname) in
  let gen_ctx' = gencontext_set_current gen_ctx (KeyO struct_name) in
  gencontext_add_prototype gen_ctx' (KeyO struct_name) (struct_name ^ ";\n");
  gencontext_add_render gen_ctx' (KeyO struct_name)
    (struct_name ^ " {\n" ^ 
       (S.concat "" (L.map (render_field gen_ctx') sinfo.sfields)) ^ 
       "};\n")

let handle_gvar gen_ctx (vinfo:var_info) =
  (* We don't render global variables in the generated transformer file *)
  ()

let render_enum_elems gen_ctx (einfo:enumeration_info) = 
  (* This is broken, if we start supporting enums, we need to render
     renamed versions of the enum values *)
  " { PLACEHOLDER_" ^ (gencontext_rename gen_ctx einfo.ename) ^ " };\n"

let handle_enum gen_ctx (einfo:enumeration_info) =
  let enum_name = "enum " ^ gencontext_rename gen_ctx einfo.ename in
  gencontext_add_prototype gen_ctx (KeyO enum_name) (enum_name ^ ";\n");
  gencontext_add_render gen_ctx (KeyO enum_name) (enum_name ^ render_enum_elems gen_ctx einfo)

let handle_union gen_ctx (sinfo:struct_info) =
  let union_name = "union " ^ (gencontext_rename gen_ctx sinfo.sname) in
  let gen_ctx' = gencontext_set_current gen_ctx (KeyO union_name) in
  gencontext_add_prototype gen_ctx (KeyO union_name) (union_name ^ ";\n");
  gencontext_add_render gen_ctx (KeyO union_name)
    (union_name ^ " {\n" ^ 
       (S.concat "" (L.map (render_field gen_ctx') sinfo.sfields)) ^
       "};\n")
  
let handle_elem gen_ctx (_, e) =
  match e with
    | Struct sinfo -> handle_struct gen_ctx sinfo
    | Union sinfo -> handle_union gen_ctx sinfo
    | Typedef tinfo -> handle_typedef gen_ctx tinfo
    | Enumeration einfo -> handle_enum gen_ctx einfo
    | Var vinfo -> handle_gvar gen_ctx vinfo      
 
(* The next section implements transformation function body generation. *)

let render_func_proto (name:string) (args : string list) renamer =
  "void " ^ name ^ "(" ^ (S.concat "," args) ^ ");\n"

let render_func (name:string) (args : string list) (body:string) renamer gen_ctx =
  "void " ^ name ^ "(" ^ (S.concat "," args) ^ ") {\n" ^ body ^"\n}\n"

let render_path_elem = function
  | PStruct s -> "struct_" ^ s
  | PUnion s -> "union_" ^ s
  | PTypedef s -> "typedef_" ^ s
  | PEnum s -> "enum_" ^ s
  | PField s -> s
  | PVar s -> 
    if S.contains s '#' then
      let idx = S.index s '#' in
      "local_" ^ (S.sub s (idx+1) ((S.length s) - (idx+1))) ^
        "_fun_" ^ (S.sub s 0 idx)
    else
      s

let deref n = "(*" ^ n ^ ")"
let addrof n = "&(" ^ n ^ ")"
let sizeof n = "sizeof(" ^ n ^ ")"

(*let render_xform_func_name e_old e_new =
  match L.hd e_old, L.hd e_new with
    | fe, te ->
      "_kitsune_transform_" ^ (render_path_elem_gen fe) ^ "_to_" ^ (render_path_elem_gen te)
*)

let render_xform_func_name e_old e_new =
  match L.hd e_old, L.hd e_new with
    | fe, te ->
      "_kitsune_transform_" ^ (render_path_elem fe) ^ "_to_" ^ (render_path_elem te)

let render_init_func_name e_new = 
  match L.hd e_new with
    | PVar k -> Ktdecl.xform_from_key k
    | _ -> failwith "Unexpected non-var argument to render_init_func_name"



(*this function counts the number of calls to XF_PTR would occur so they can be optimized *)
let rec countderef gen_ctx t0 t1 : int =
    dbg_print "COUNT_DEREF ";
    match t0, t1 with
      | TNamed (nm, _), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "Named,_";
          countderef gen_ctx (H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))) t1
      | _, TNamed (nm, _) when H.mem gen_ctx.td_incomplete (KeyTd (new_rename nm)) ->dbg_print "_, Named";
          countderef gen_ctx t0 (H.find gen_ctx.td_incomplete (KeyTd (new_rename nm))) 
      | TPtr (TNamed (nm, _), gen_name, mem_fns), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "TPtr, _when";
          countderef gen_ctx (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) t1
      | _, TPtr (TNamed (nm, _), gen_name, mem_fns) when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "_, TPtr";
          countderef gen_ctx t0 (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) 

      (*| TPtr (_, Some (GenName gv0), _), TPtr (_, Some (GenName gv1), _) -> print_endline "TPtr, TPtr"; 1*)
      | TNamed (nm, _), _ ->dbg_print "Named,_"; 0
      | TFun _, TFun _ ->dbg_print "TFun TFun";
       0
      | TPtr (TFun _, _, _), TPtr (TFun _, _, _) ->dbg_print "TPtr TFun, TPtr TFun: ";
               0
      | TPtr (t0', _, _), TPtr (t1', _, _) ->dbg_print "TPtr t0, TPtr, t1";
        (match t0' with
          | TVoid None -> dbg_print "OPQ PTR"; 0
          | _ -> 1 + (countderef gen_ctx t0' t1') )
      | TPtrArray (t0', len0, _), TPtrArray (t1', len1, _) -> dbg_print "TPtrAray";
          (*1 +*) (countderef gen_ctx  t0' t1') (* This pointer will be handled separately*)  
      | TArray (t0', len0), TArray(t1', len1) -> dbg_print "TArray"; (*0*)
          (countderef gen_ctx  t0' t1') (* get the pointer situation on the array elements*)
      | _ ->dbg_print "no deref"; 0

(* get only the function name to enqueue*)  (*TODO This is mostly dead code *)
let rec generate_enqueue gen_ctx t0 t1 compare_ctx : string =
    match t0, t1 with
      | TPtr (_, Some (GenName gv0), _), TPtr (_, Some (GenName gv1), _) -> dbg_print "TPtr, TPtr";
        assert (gv0 = gv1);
        (*"transform_invoke_closure"*) (*TODO*) "transform_prim"

      (* Ultimately, the following two cases will produce errors.
         Need to decide whether to issue the errors here or wait
         until they occur in the recursive call... for now, the
         choice is to wait. *)
      | TNamed (nm, _), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "Named,_";
          generate_enqueue gen_ctx (H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))) t1 compare_ctx
      | _, TNamed (nm, _) when H.mem gen_ctx.td_incomplete (KeyTd (new_rename nm)) ->dbg_print "_, Named";
          generate_enqueue gen_ctx t0 (H.find gen_ctx.td_incomplete (KeyTd (new_rename nm))) compare_ctx

      | TPtr (TNamed (nm, _), gen_name, mem_fns), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "TPtr, _when";
          generate_enqueue gen_ctx (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) t1 compare_ctx
      | _, TPtr (TNamed (nm, _), gen_name, mem_fns) when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "_, TPtr";
          generate_enqueue gen_ctx t0 (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) compare_ctx

      | TFun _, TFun _ ->dbg_print "TFun TFun";
          (* make this an assertion as it is unexpected behavior *)
          (* genError gen_ctx "Cannot generate code for a fun -> fun transformation." *)
      (*this is a macro to    "(assert(0),NULL)"*)
        "transform_fptr"

      | TPtr (TFun _, _, _), TPtr (TFun _, _, _) ->dbg_print "TPtr TFun, TPtr TFun: ";
        (*"XF_FPTR()"*)
        "transform_fptr"

      | TPtr (t0', _, _), TPtr (t1', _, _) ->dbg_print "TPtr t0, TPtr, t1";
        (*"XF_PTR(" ^ (generate_xform_sm gen_ctx t0' t1' len_base) ^ ")"*)
        (generate_enqueue gen_ctx t0' t1' compare_ctx) 

      | TPtrArray (t0', len0, _), TPtrArray (t1', len1, _) -> dbg_print "TPtrArray";
        assert(len0 = len1); (* this is probably not quite right *)
        (generate_enqueue gen_ctx t0' t1' compare_ctx) 
      | TArray (t0', len0), TArray(t1', len1) -> dbg_print "TArray";
        assert(len0 = len1); (* this is probably not quite right *)
        (generate_enqueue gen_ctx t0' t1' compare_ctx) 
      | TInt _, TInt _
      | TFloat _, TFloat _
      | TEnum _, TEnum _
      | TBuiltin_va_list, TBuiltin_va_list
      | TUnion _, TUnion _
      | TPtrOpaque _, TPtrOpaque _ ->dbg_print "IntFloatEnUnOPaq";
        (*"XF_RAW(" ^ sizeof (render_type (gencontext_set_renamer gen_ctx new_rename) t1 false None 1) ^ ")"*)
        "transform_prim"
      | _ ->dbg_print "Empty _ENQ";
        match (path_elem_from_type t0) with
          | None -> (
            let unrenderable =  (render_type gen_ctx t0 false false None MacroOnly ) in
            Format.printf "WARNING: no transformation function could be found for %s \n" unrenderable;
           "transform_prim")
          | _ ->  (
        let path_elem0, _ = option_get_unsafe (path_elem_from_type t0) in
        let path_elem1, _ = option_get_unsafe (path_elem_from_type t1) in
            let target_fun = render_xform_func_name [path_elem0] [path_elem1] in
            gencontext_add_dep gen_ctx false (KeyFn target_fun); target_fun)

type generic_type =
  | GenOk of string (* use as-is*)
  | GenMakeMe  (* need to propagate gen args *)
  | GenNone (* not generic*)

let rec is_gent_type gd0  =
  match gd0  with
    | OtherGenericBinding (GenName n0) -> true 
    | ProgramType t0 -> false
    | PtrTo gd0'->
      (is_gent_type gd0') 


(* TODO consider removing t1orig t0orig...not sure how arrays will work so leaving ofr now.....*)
let rec has_generics' gen_ctx t0 t1 i t0orig t1orig : generic_type =
let render_outer_type gd0 gd1 : string = 
  (generate_genin_xform_fullinner_type gen_ctx gd0 gd1 false) 
in
(* this function determines if closures are (currently) necessary *)
    match t0, t1 with
      (* look deeper *)
      | TPtrArray (t0', _, _), TPtrArray (t1', _, _)
      | TArray (t0', _), TArray(t1', _)
      | TPtr (t0', _, _), TPtr (t1', _, _) ->
          dbg_print "Render_outer_type call has_gen";
          has_generics' gen_ctx t0' t1' (i+1) (Some t0) (Some t1)
      | TNamed (nm, _), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "Named,_";
          has_generics' gen_ctx (H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))) t1 i None None 
      | _, TNamed (nm, _) when H.mem gen_ctx.td_incomplete (KeyTd (new_rename nm)) ->dbg_print "_, Named";
          has_generics' gen_ctx t0 (H.find gen_ctx.td_incomplete (KeyTd (new_rename nm))) i None None
      | TPtr (TNamed (nm, _), gen_name, mem_fns), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "TPtr, _when";
          has_generics' gen_ctx (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) t1 i None None
      | _, TPtr (TNamed (nm, _), gen_name, mem_fns) when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->dbg_print "_, TPtr";
          has_generics' gen_ctx t0 (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) i None None
      (* does not use *)
      | TFun _, TFun _
      | TInt _, TInt _
      | TFloat _, TFloat _
      | TEnum _, TEnum _
      | TBuiltin_va_list, TBuiltin_va_list
      | TUnion _, TUnion _
      | TVoid None, TVoid None 
      | TPtrOpaque _, TPtrOpaque _ -> dbg_print "NONE1"; GenNone
      (* totally generics *)
      | TVoid (Some (GenName gv0)), TVoid (Some (GenName gv1)) -> 
        (GenOk (compute_gen_arg_var_name gv0))
      (* (test for) generics *)
      | _ ->
          dbg_print "Generics.  Getting unsafe.\n";
        let path_elem0, _ = option_get_unsafe (path_elem_from_type t0) in
        let path_elem1, _ = option_get_unsafe (path_elem_from_type t1) in
        let target_fun = render_xform_func_name [path_elem0] [path_elem1] in
        let target_fun_reg = render_xform_func_name [path_elem0] [path_elem1] in
        let generic_args = gencontext_get_xform_generics gen_ctx (KeyFn target_fun_reg) in
           (dbg_print ("target_fun:" ^ target_fun ^ " len = " ^ string_of_int (L.length generic_args));
           if (L.length generic_args) = 0 then (dbg_print "NONE3";GenNone )
           else 
           (let gen_in0 = getTypeGenIns t0 in
           let gen_in1 = getTypeGenIns t1 in
           let is_gen_t = is_gent_type (L.hd gen_in0) in
           if (is_gen_t) then (GenMakeMe ) (* this just passes the type along...'gen' isthe name of the propogated arg. *)
           else
           begin
           let maintype = (append_deref gen_ctx  (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) t0 false false None MacroOnly) i) in
           let maintype_out = (append_deref gen_ctx  ((render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) t0 false false None MacroOnly)^"_OUT") i) in
           let targettype = match i with
              | 0 -> "-1"
              | _ -> ((append_deref gen_ctx  (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) t0 false false None MacroOnly) (i-1)) ^ 
                      "_OF_" ^ (S.concat "__" (L.map2 render_outer_type gen_in0 gen_in1))) in
           let targettype_out = match i with
              | 0 -> "-1"
              | _ -> ((append_deref gen_ctx  (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) t0 false false None MacroOnly) (i-1)) ^ 
                      "_OF_" ^ (S.concat "__" (L.map2 render_outer_type gen_in0 gen_in1)) ^ "_OUT") in
           let sz0 = render_sizeof gen_ctx t0 "_kitsune_old_rename_" i in (*why is "renamer" not working here?  whatever. *)
           let sz1 = render_sizeof gen_ctx t1 "_kitsune_new_rename_" i in
           dbg_print "On to gen args....\n%!";
           let genren =  (maintype  ^ "_OF_" ^ (S.concat "__" (L.map2 render_outer_type gen_in0 gen_in1)))    
           in
           if not (L.exists (fun x-> x = (genren^ ", ")) !(gen_ctx.macrotypes)) then
             begin
               let arglens = (string_of_int (L.length gen_in0)) in
               (* This is obviously not ideal.  However, nested generics...also not ideal. *)
               let argmess = "
                  typ janky"^genren ^"["^ arglens ^"] = {" ^ (S.concat ", " (L.map2 render_outer_type gen_in0 gen_in1)) ^"};
                  args_arr["^genren^"] = malloc(sizeof(typ)* "^arglens ^ ");//TODO FREEING 
                  memcpy(args_arr[" ^genren ^"], janky"^ genren^", (sizeof(typ)* "^arglens ^ "));
               " in
               gen_ctx.stored_num_gen_args := L.append !(gen_ctx.stored_num_gen_args) ["  num_gen_args_arr["^genren^"] = "^arglens^";\n"];
               gen_ctx.stored_gen_args := L.append !(gen_ctx.stored_gen_args) [argmess];
               dbg_print ("after args with i =" ^(string_of_int i) ^"with basetype =" ^ maintype ^ "and target = ^" ^ target_fun ^"genren = "^ genren ^" and isgent = " ^(string_of_bool is_gen_t));
               populate_type_info gen_ctx (genren) (maintype )  (targettype )  ("0") 
                  ("0") (sz0) (target_fun);
               populate_type_info gen_ctx (genren^"_OUT") (maintype_out)  (targettype_out)  ("0") 
                  ("0") (sz1) (target_fun);

               if ( (i!=0) && (not (L.exists (fun x-> x = (targettype^ ", ")) !(gen_ctx.macrotypes)))) then
               begin
                 gen_ctx.stored_num_gen_args := L.append !(gen_ctx.stored_num_gen_args) ["  num_gen_args_arr["^targettype^"] = "^arglens^";\n"];
               (* This is obviously not ideal.  However, nested generics...also not ideal. *)
               let argmess = "
                    typ janky"^targettype ^"["^ arglens ^"] = {" ^ (S.concat ", " (L.map2 render_outer_type gen_in0 gen_in1)) ^"};
                    args_arr["^targettype^"] = malloc(sizeof(typ)* "^arglens ^ ");
                    memcpy(args_arr[" ^targettype ^"], janky"^ targettype^", (sizeof(typ)* "^arglens ^ "));
               " in
                 gen_ctx.stored_gen_args := L.append !(gen_ctx.stored_gen_args) [argmess];
                 let sz0' = render_sizeof gen_ctx t0 "_kitsune_old_rename_" (i-1) in (*why is "renamer" not working here?  whatever. *)
                 let sz1' = render_sizeof gen_ctx t1 "_kitsune_new_rename_" (i-1) in
                 let (t0orig', t1orig') = (match (t0orig, t1orig) with | Some t0f, Some t1f -> (t0f, t1f) | _ -> t0,  t1) in
                 let target_fun' = ( match (path_elem_from_type t0orig') with 
                   | Some o ->
                    (let path_elem0', _ = option_get_unsafe (path_elem_from_type t0orig') in
                    let path_elem1', _ = option_get_unsafe (path_elem_from_type t1orig') in
                    render_xform_func_name [path_elem0'] [path_elem1'] )
                   | _ -> target_fun 
                 ) in 
                 let maintype' = ((append_deref gen_ctx  (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) t0 false false None MacroOnly) (i-1))) in
                 dbg_print ("after renamer with i =" ^(string_of_int i) ^"with basetype =" ^ maintype' ^ "and target = ^" ^ target_fun' ^"targtyp = "^ targettype ^" and isgent = " ^(string_of_bool is_gen_t));
                 populate_type_info gen_ctx (targettype) (maintype')  ("-1") (genren)  (*TODO double check -1 for target*)
                    ("0") (sz0') (target_fun');
                 populate_type_info gen_ctx (targettype^"_OUT") (maintype'^"_OUT")  ("-1")  (genren)  (*TODO double check -1 for target*)
                    ("0") (sz1') (target_fun');
match (t0, t1) with
      | TNamed (nm1, _), TNamed (nm2, _) -> (dbg_print "TNAMED")
      | _ -> ()
               end
             end
             else
               dbg_print ("TNamed with i =" ^(string_of_int i) ^"with basetype =" ^ maintype ^ "and target = ^" ^ target_fun ^"genren = "^ genren ^" and isgent = " ^(string_of_bool is_gen_t));
             GenOk genren 
           end
          ))
and
 generate_genin_xform_fullinner_type gen_ctx gd0 gd1 isrecall: string =
  match gd0, gd1 with
    | OtherGenericBinding (GenName n0), OtherGenericBinding (GenName n1) -> (*"@" *)
        assert (n0 = n1);
        (compute_gen_arg_var_name n0) (*gen_t*)
           (*mark that we dont want anything with gen_t in it for now*)
    | ProgramType t0, ProgramType t1 ->
    (match (has_generics' gen_ctx t0 t1 1 None None) with
        | GenOk t -> dbg_print ("REC SOME GEN OK"^ t); t
        | GenMakeMe  -> dbg_print ( "REC SOME GEN render"); "NOT IMPLEMENTED" (*TODO tell me this isn't possible, righT??*)
        | GenNone -> dbg_print ("NO  GEN render " ^ (render_type gen_ctx t0 false isrecall None MacroOnly)); 
               ( render_type gen_ctx t0 false isrecall None MacroOnly) )
    | PtrTo gd0', PtrTo gd1' ->  dbg_print "generate_genin_xform_fullinner_type PtrTo";
       append_deref gen_ctx  (generate_genin_xform_fullinner_type gen_ctx gd0' gd1' true)  1
    | _ -> genError gen_ctx "Unexpected: unmatched generic descriptors"

let has_generics gen_ctx t0 t1  : generic_type =
 has_generics' gen_ctx t0 t1 0 None None


(* Deprecated*)
let rec render_usercode gen_ctx compare_ctx (repl_in:string option) (repl_out:string option) (repl_base:string option) (code:string) =
  ""

let rec check_toplevel_type pred (t0:typ) (t1:typ) =
  let p0 = Kttcompare.path_elem_from_type t0 in
  let p1 = Kttcompare.path_elem_from_type t1 in
  match p0, p1 with
    | Some (p0, _), Some (p1, _) -> pred ([p0], [p1])      
    | _, _ -> 
      match t0, t1 with
        | TArray(t0', l0), TArray(t1', l1) ->
          l0 = l1 && check_toplevel_type pred t0' t1'
        | _, _ -> false
;;
      

let len_to_string len len_base = match len with
  | Len_Unknown -> 
    failwith "Cannot generate serialization when the field length is unknown"
  | Len_Int l -> Int64.to_string l 
  | Len_Field l -> 
    (match len_base with
      | None -> l
      | Some s -> s ^ "." ^ l)
  | Len_Nullterm ->
    failwith "Not yet handled"

(* see array, array2, array3, array4 in /tests/ktcc-profile for example of arrays. *)
type arr_type =
  | ArrayPlain of array_len * array_len  (* an array.  (this may be an array of pointerers, or an array of non-pointers...but the array itself is not a pointer) *)
  | ArrayPtr of array_len * array_len  (* pointer to an array (this may a pointer to an array of pointers, or a pointer to an array of non pointers *)
  | ArrayNone (* not array*)

let rec has_union gen_ctx t0 t1 : bool = 
(* this function tests for unions (so that we skip generating) *)
    match t0, t1 with
      (* obviously a union. *)
      | TUnion (_, _),TUnion (_, _) -> true
      (* look deeper *)
      | TPtrArray (t0', _, _), TPtrArray (t1', _, _) 
      | TPtr (t0', _, _), TPtr (t1', _, _) -> 
          has_union gen_ctx t0' t1' 
      | TNamed (nm, _), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) -> 
          has_union gen_ctx (H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))) t1
      | _, TNamed (nm, _) when H.mem gen_ctx.td_incomplete (KeyTd (new_rename nm)) ->
          has_union gen_ctx t0 (H.find gen_ctx.td_incomplete (KeyTd (new_rename nm))) 
      | TPtr (TNamed (nm, _), gen_name, mem_fns), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->
          has_union gen_ctx (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) t1
      | _, TPtr (TNamed (nm, _), gen_name, mem_fns) when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->
          has_union gen_ctx t0 (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns))
      (* does not use *)
      | _ -> false

let rec use_array gen_ctx t0 t1 : arr_type = 
(* this function determines if closures are (currently) necessary *)
    match t0, t1 with
      (* uses arrays *)
      | TPtrArray (t0', len0, _), TPtrArray (t1', len1, _) -> ((dbg_print ("USE_ARRAY PTR ARRAY "^ (len_to_string len0 None))); ArrayPtr (len0, len1))
      | TArray (t0', len0), TArray(t1', len1) -> ((dbg_print ("USE_ARRAY ARRAY (%s)\n"^ (len_to_string len0 None))); ArrayPlain (len0, len1))
      (* look deeper *)
      | TPtr (t0', _, _), TPtr (t1', _, _) ->(dbg_print "USE_ARRAY TPTR  ");
          use_array gen_ctx t0' t1' 
      | TNamed (nm, _), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->(dbg_print "USE_ARRAY TNAMED A ");
          use_array gen_ctx (H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))) t1
      | _, TNamed (nm, _) when H.mem gen_ctx.td_incomplete (KeyTd (new_rename nm)) ->(dbg_print "USE_ARRAY TNAMED B ");
          use_array gen_ctx t0 (H.find gen_ctx.td_incomplete (KeyTd (new_rename nm)))
      | TPtr (TNamed (nm, _), gen_name, mem_fns), _ when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->(dbg_print "USE_ARRAY TNAMED C ");
          use_array gen_ctx (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns)) t1
      | _, TPtr (TNamed (nm, _), gen_name, mem_fns) when H.mem gen_ctx.td_incomplete (KeyTd (old_rename nm)) ->(dbg_print "USE_ARRAY TNAMED D \n");
          use_array gen_ctx t0 (TPtr ((H.find gen_ctx.td_incomplete (KeyTd (old_rename nm))), gen_name, mem_fns))
      (* does not use *)
      | _ -> ArrayNone



let generate_trans_func calling_gen_ctx compare_ctx e =
  gen_decls :=  "";
  let generate_field_xform gen_ctx full_xform old_name new_name m =
    match m with
      | Extern -> ""
      | MatchInit (me1, _, code) -> dbg_print "MatchInit";
        begin
          assert full_xform;
          match me1 with
            | PField fname1 :: _ ->
              let gen_ctx = gencontext_append_symbol gen_ctx "" fname1 in
              "{" ^ (render_usercode gen_ctx compare_ctx None (Some (new_name ^ "->" ^ fname1)) 
                       (Some (deref old_name)) code) ^ "}\n"
            | _ -> genError gen_ctx "Unexpected: paths for fields should be PFields"
        end

      | MatchAuto ((me0, me1), (t0, t1), _, MatchVar (_, _)) -> dbg_print "MatchAuto in trans_func";
        begin
          let deref_old, deref_new = deref old_name, deref new_name in
          match me0, me1 with
            | PField fname0 :: _, PField fname1 :: _ -> dbg_print "PField fname0 fname1";
              let gen_ctx = gencontext_append_symbol gen_ctx fname0 fname1 in
                    current_name_field :=  fname0;
   
    if ((has_union gen_ctx t0 t1) = true) then 
             (Format.printf "!!!!!!!!!!! WARNING: SKIPPING: %s \n" !(current_name); "")
    else(
    ( let typ = (match (has_generics gen_ctx t0 t1 ) with
        | GenOk t -> dbg_print "AUTO SOME GEN\n"; t
        | GenMakeMe -> 
                   let typename = (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0) false false (None) MacroOnly)  (countderef gen_ctx  t0 t1))  in  
                   dbg_print "AUTO SOME GEN MAKING"; "mktypfromgenargs(" ^ typename ^ ", num_gen_args, args)"
        | GenNone -> dbg_print "NO  GEN\n"; 
                   gencontext_set_xform gen_ctx (generate_enqueue gen_ctx t0 t1 compare_ctx);
                   (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0) false false (None) MacroOnly)  (countderef gen_ctx  t0 t1)) ) in
     let finaltyp = (match (use_array gen_ctx t0 t1) with 
                       | ArrayPlain (len0, len1) -> ("mktyparr("^ (len_to_string len0 (Some deref_old)) ^", "^typ^")")   (*TODO what to do with len1?!?!*)
                       | ArrayPtr (len0, len1) -> ("mktypptr(mktyparr("^ (len_to_string len0 (Some deref_old)) ^", "^typ^"))")   (*TODO what to do with len1?!?!*)
                       | ArrayNone ->  (typ)) in 
     dbg_print ">>>>>>>>>>>>>>>>>>Working on  AAA\n";
     let ret  = "  visit(" ^ (addrof (deref_old ^
        "." ^ fname0)) ^ ", " ^ finaltyp ^ ", " ^ (addrof (deref_new ^ "." ^ fname1)) ^ ");\n"
     in
     dbg_print ("RETURNED from AAA ENQ"^ret); ret
))
            | _ -> genError gen_ctx "Unexpected: paths for fields should be PFields"
        end

      | MatchAuto ((me0, me1), (t0, t1), _, _) ->
        failwith "generate_field_xform: unepected elem type"
          
      | UnmatchedDeleted _ -> ""
      | UnmatchedAdded _ | UnmatchedType _ | UnmatchedError _ ->
        failwith "Unexpected unmatched element found in generate_trans_fun"
  in
  let generate_auto_body gen_ctx full_xform ptr_xform in_var out_var t0 t1 (g0_out, g1_out) minfo =
    match minfo with
      | MatchStruct (order_changed, field_matches) -> dbg_print "MatchStruct";
        S.concat "" (L.map (generate_field_xform gen_ctx full_xform in_var out_var) field_matches)
      | MatchUnion (field_matches) ->  
             Format.printf "!!!!!!!!!!! WARNING: SKIPPING: %s \n" !(current_name); ""
      | MatchTypedef (t0', t1') -> dbg_print "MatchTypedef"; 
    let typ = (match (has_generics gen_ctx t0' t1' ) with
        | GenOk t -> dbg_print " TYPED SOME GEN\n"; t
        | GenMakeMe -> 
                   let typename = (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0') false false (None) MacroOnly)  (countderef gen_ctx  t0' t1'))  in  
                   dbg_print"MatchTypedef GENMAKEME\n"; "mktypfromgenargs(" ^ typename ^ ", num_gen_args, args)"
        | GenNone -> dbg_print "NO  GEN\n"; 
                   gencontext_set_xform gen_ctx (generate_enqueue gen_ctx t0' t1' compare_ctx);
                   (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0') false false (None) MacroOnly)  (countderef gen_ctx  t0' t1')))  in  

     if ((has_union gen_ctx t0 t1) = true) then 
      (Format.printf "!!!!!!!!!!! WARNING: SKIPPING: %s \n" !(current_name); "") else
     (let finaltyp = (match (use_array gen_ctx t0' t1') with 
                       | ArrayPlain (len0, len1) -> ("mktyparr("^ (len_to_string len0 None) ^", "^typ^")")   (*TODO what to do with len1?!?!*)
                       | ArrayPtr (len0, len1) -> ("mktypptr(mktyparr("^ (len_to_string len0 None) ^", "^typ^"))")   (*TODO what to do with len1?!?!*)
                       | ArrayNone ->  (typ)) in
          dbg_print ">>>>>>>>>>>>>>>>>>Working on  FFF\n";
          let enqstr = "  visit(" ^ in_var ^ ", " ^ finaltyp ^ ", " ^ out_var ^ ");" in 
          enqstr)

      | MatchVar (g0_in, g1_in) -> dbg_print "MatchVar";
                       (
                        (*"printf(\"enqueing " ^generate_enqueue gen_ctx t0 t1 compare_ctx ^ "\\n\");\n" ^ *)
    let typ = (match (has_generics gen_ctx t0 t1 ) with
        | GenOk t -> dbg_print " VAR SOME GEN\n"; t
        | GenMakeMe -> 
                   let typename = (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0) false false (None) MacroOnly)  (countderef gen_ctx  t0 t1))  in  
                   dbg_print "VAR SOME GEN MAKEME";  "mktypfromgenargs(" ^ typename ^", gen_t->num_gen_args, gen_t->args)"
        | GenNone -> dbg_print "NO  GEN\n"; 
                         gencontext_set_xform gen_ctx (generate_enqueue gen_ctx t0 t1 compare_ctx);
                   (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0) false false (None) MacroOnly)  (countderef gen_ctx  t0 t1)))  in  
    if ((has_union gen_ctx t0 t1) = true) then 
     (Format.printf "!!!!!!!!!!! WARNING: SKIPPING: %s \n" !(current_name); "") else
    (let finaltyp = (match (use_array gen_ctx t0 t1) with 
                       | ArrayPlain (len0, len1) -> ("mktyparr("^ (len_to_string len0 None) ^", "^typ^")")   (*TODO what to do with len1?!?!*)
                       | ArrayPtr (len0, len1) -> ("mktypptr(mktyparr("^ (len_to_string len0 None) ^", "^typ^"))")   (*TODO what to do with len1?!?!*)
                       | ArrayNone -> typ) in
    dbg_print ">>>>>>>>>>>>>>>>>>Working on  HHH\n";
			 "  visit(" ^in_var ^ ", " ^ finaltyp ^", "^ out_var ^ ");\n"))

      | MatchEnum ->
    let typ = (match (has_generics gen_ctx t0 t1 ) with
        | GenOk t -> dbg_print " ENUM SOME GEN"; t
        | GenMakeMe -> 
                   let typename = (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0) false false (None) MacroOnly)  (countderef gen_ctx  t0 t1))  in  
                   dbg_print "ENUM SOME GEN, TODO NOT TESTED"; "mktypfromgenargs(" ^ typename ^ ", gen_t->num_gen_args, gen_t->args)"
        | GenNone -> dbg_print "ENUM NO  GEN"; 
                   gencontext_set_xform gen_ctx (generate_enqueue gen_ctx t0 t1 compare_ctx);
                   (append_deref gen_ctx (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) 
                                 (t0) false false (None) MacroOnly)  (countderef gen_ctx  t0 t1)))  in  
     if ((has_union gen_ctx t0 t1) = true) then 
             (Format.printf "!!!!!!!!!!! WARNING: SKIPPING: %s \n" !(current_name); "") else
     (let finaltyp = (match (use_array gen_ctx t0 t1) with 
                       | ArrayPlain (len0, len1) -> ("mktyparr("^ (len_to_string len0 None) ^", "^typ^")")   (*TODO what to do with len1?!?!*)
                       | ArrayPtr (len0, len1) -> ("mktypptr(mktyparr("^ (len_to_string len0 None) ^", "^typ^"))")   (*TODO what to do with len1?!?!*)
                       | ArrayNone -> typ) in
	"//KKKKKK\n  visit(" ^in_var ^ ", " ^ finaltyp ^", "^ out_var ^ ");\n")
  in
  let generics_decls arg_var num_args_var generics =
    let create_generic_local idx gen_name =
      "  typ " ^ (compute_gen_arg_var_name gen_name) ^ " = args[" ^ (string_of_int idx) ^ "];\n"
    in
    let numargs = (if ((L.length generics) != 0) then ((L.length generics)) else 0) in
"  typ * args = cstrider_get_generic_args(gen); 
  int num_gen_args = cstrider_get_num_gen_args(gen);
  assert(num_gen_args == " ^ (string_of_int numargs) ^ ");\n" ^
      (S.concat "" (map_index create_generic_local generics))
  in
  match e with
    | Extern -> ()
    | MatchInit (([PVar key_new] as me1), t1, code) -> dbg_print "MatchInit in match e";
      begin
        let var_out = "local_out" in
        current_name :=  key_new;
        current_name_field :=  "";
        let fname = render_init_func_name me1 in
        let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in
        let gen_ctx = gencontext_append_symbol gen_ctx "" key_new in
        let lookup_new =
          ("  " ^render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false false (Some var_out) AddDep) ^ ";\n" ^
            var_out ^ " = " ^ Ktdecl.render_get_val_new_call key_new ^ ";\n" ^
            "  assert(" ^ var_out ^ ");\n"
        in
        let body = lookup_new ^ 
          "{" ^ (render_usercode gen_ctx compare_ctx None (Some (deref var_out)) None code) ^ "}"
        in
        let proto = render_func_proto fname [] id in
        let impl = render_func fname [] body id gen_ctx in
        gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
      end

    | MatchAuto ((([PVar key_old] as me0), ([PVar key_new] as me1)), (t0, t1), generics, MatchVar (g0_in, g1_in)) -> dbg_print "MatchAuto in match e";

      let full_xform = Hashtbl.mem compare_ctx.requires_full_xform (me0, me1) in
      let ptr_xform = Hashtbl.mem compare_ctx.requires_shallow_xform (me0, me1) in
      current_name :=  key_old;
      current_name_field :=  "";
      let (var_in, var_out) = ("local_in", "local_out") in
      let fname = render_init_func_name me1 in
      let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in
      let gen_ctx = gencontext_append_symbol gen_ctx key_old key_new in
      gen_ctx.visitall := L.append !(gen_ctx.visitall) [fname^", "];
      let lookup_old =
        "  " ^ (render_type (gencontext_set_renamer gen_ctx old_rename) (ptrTo t0) false false (Some var_in) AddDep) ^ ";\n  " ^
          var_in ^ " = " ^ Ktdecl.render_get_val_old_call key_old ^ ";\n" ^
          "  assert(" ^ var_in ^ ");\n"
      in
      let decl =
        (render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false false (Some var_out) AddDep) ^ ";\n  " 
      in let lookup_new = "  " ^ decl ^
          var_out ^ " = " ^ Ktdecl.render_get_val_new_call key_new ^ ";\n" ^
          "  assert(" ^ var_out ^ ");\n"
      in
      (* for cstrider, list all of the function names to register.*)
      ( let i = String.index decl ' ' in
      let parse =(String.sub decl 0 i) in 
      dbg_print parse;
      (* yuck. the types are baked in to "render_type", must string parse. :(  *)
      let realtype = 
         (match  parse with
           | "struct" -> (String.sub decl 0 (String.index_from decl ((String.length "struct")+1) ' '));
           | _ -> parse ) in
      gen_ctx.externs_to_reg := L.append !(gen_ctx.externs_to_reg) ["extern " ^realtype ^" "^key_new^";"]);
      let xform_call = 
        (*if full_xform || ptr_xform then*)
          generate_auto_body gen_ctx full_xform ptr_xform var_in var_out t0 t1 generics (MatchVar (g0_in, g1_in))
        (*else ""*)
      in
      let body = lookup_old ^ lookup_new ^ xform_call in
      let proto = render_func_proto fname [] id in
      let impl = render_func fname [] body id gen_ctx in
      gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
        
    | MatchAuto ((me0, me1), (t0, t1), generics, minfo) -> dbg_print "MatchAuto in match e";
      let full_xform = Hashtbl.mem compare_ctx.requires_full_xform (me0, me1) in
      let ptr_xform = Hashtbl.mem compare_ctx.requires_shallow_xform (me0, me1) in      
          let (var_in, var_out, num_args_var, args_var) = ("arg_in", "arg_out", "num_args", "args") in
          let local_in, local_out = ("local_in", "local_out") in
          let fname = render_xform_func_name me0 me1 in
          current_name := ((render_path_elem (List.hd me0)));
          current_name_field :=  "";
          let gen_ctx = gencontext_set_current calling_gen_ctx (KeyFn fname) in      
          let gen_ctx = gencontext_append_symbol gen_ctx (Xflang.path_to_string me0) (Xflang.path_to_string me1) in
          let args = [ "void *" ^ var_in; "void *" ^ var_out; "typ gen"] in

               (*TODO make these return unit *)
               let _ = (render_type (gencontext_set_renamer gen_ctx in_MACRO_rename) (t0) false false (None) AddMacro) in ();
               let _ = (render_type (gencontext_set_renamer gen_ctx out_MACRO_rename) (t1) false false (None) AddMacro) in ();
               gen_ctx.macrotypes_funs := L.append !(gen_ctx.macrotypes_funs) [(generate_enqueue gen_ctx t0 t1 compare_ctx) ^ ", "];
               gen_ctx.macrotypes_funs := L.append !(gen_ctx.macrotypes_funs) [(generate_enqueue gen_ctx t0 t1 compare_ctx) ^ ", "];
               let _ = (render_type (gencontext_set_renamer gen_ctx old_rename) (t0) false false (None) AddSizeOf) in ();
               let _ = (render_type (gencontext_set_renamer gen_ctx new_rename) (t1) false false (None) AddSizeOf) in ();
          let local_init = 
            "  "^ render_type (gencontext_set_renamer gen_ctx old_rename) (ptrTo t0) false false (Some local_in) AddDep ^ " = " ^ var_in ^ ";\n" ^
            "  "^ render_type (gencontext_set_renamer gen_ctx new_rename) (ptrTo t1) false false (Some local_out) AddDep ^ " = " ^ var_out ^ ";\n"
          in
          gen_decls := (generics_decls args_var num_args_var (gencontext_get_xform_generics gen_ctx (KeyFn fname)));
          let body =
            local_init ^
              (generics_decls args_var num_args_var (gencontext_get_xform_generics gen_ctx (KeyFn fname))) ^
              (generate_auto_body gen_ctx full_xform ptr_xform local_in local_out t0 t1 generics minfo); (*TODO get rid of this!*)
          in
          let proto = render_func_proto fname args id in
          let impl = render_func fname args body id gen_ctx in
          gencontext_add_xformer gen_ctx (KeyFn fname) proto impl
       (* end*)
       

    | UnmatchedDeleted me -> ()
    | UnmatchedAdded _ | UnmatchedType _ | UnmatchedError _ | MatchInit _ -> 
      failwith "Unexpected unmatched element found in generate_trans_fun"


let grab_generic_sigs gen_ctx = function
  | MatchInit _ | UnmatchedDeleted _ | UnmatchedAdded _ | UnmatchedType _ | Extern 
  | UnmatchedError _ | MatchAuto (_, _, _, MatchVar (_, _)) ->
    ()
  | MatchAuto ((me0, me1), _, generics, _) ->
    gencontext_set_xform_generics gen_ctx (KeyFn (render_xform_func_name me0 me1)) generics


let handle_undefined_types gen_ctx =
  let iter_fun _ dep_key =
    if not (H.mem gen_ctx.rendered dep_key) then
      match dep_key with
        | KeyTd name -> 
          print_endline ("ERROR: cannot generate prototype for typedef: " ^ name)
        | _ -> 
          H.add gen_ctx.prototypes dep_key (elem_key_to_s dep_key ^ ";")            
  in
  H.iter iter_fun gen_ctx.soft_deps;
  H.iter iter_fun gen_ctx.hard_deps



(* This adds all of the global root registration functions *)
(* i.e.:  kitsune_register_var("listptr", 0, 0, 0, & listptr, sizeof(listptr), 1); *)
let insert_register_vars_cstrider chan chan_hdr=
  (* insert extern prototypes for function registration*)
  let ic = open_in "externfuns.txt" in
  (try
   while true do
     let line = input_line ic in
     output_string chan (line^"\n")
   done;
   () 
  with
   End_of_file -> close_in ic);
  output_string chan "void cstrider_register_init(void){ \n";
  (* Insert all of the registration functions from (ktglobalreg.ml)'s output *)
  let ic = open_in "register_vars.txt" in
  (try
   while true do
     let line = input_line ic in
     output_string chan ("  "^line^"\n")
   done;
   ()
  with
   End_of_file -> close_in ic);
  output_string chan "}\n"

let insert_register_externs_cstrider chan chan_hdr funnames=
  let rec insert_ext' l =
  match l with
    | [] ->()
    | h::t ->  (output_string chan (h^"\n")); insert_ext' t 
  in
  insert_ext' funnames

(* this allows for easier linking...defining the types as integers (in addtion to enum for compilation *)
let insert_types chan typelist = 
  let rec insert_types' l i  =
  match l with
    | [] ->()
    | h::t ->
         (* Ugh, stripping out the commas. TODO don't hardcode commas.....duh.*)
         let decl = (String.sub h 0 ((String.length h)-2)) ^ 
            "_i = " ^(string_of_int i) ^ ", " in (output_string chan decl); insert_types' t (i+1)
  in
  output_string chan "int ";
  insert_types' typelist 0;
  output_string chan "TERMINATOR_i = -1;\n"

let elem_key_to_s_placeholder gen_ctx t = 
  match t with 
    | KeyO s -> 
        (* Generate a placeholder for external type *)
        (let new_renamer = "struct _kitsune_new_rename_" in 
        let choplen = (String.length new_renamer ) in
        (if new_renamer =  (String.sub s 0 choplen ) then
          "int "^(render_type gen_ctx (TStruct ((String.sub s (choplen) ((String.length s) - choplen) ), [])) 
                 false false None MacroOnly)^" = -1;\n"^s^"{};\n"
        else s^"{};\n"))
    | _ -> ""

let write_ordered_symbols (chan:out_channel) gen_ctx =
  let written = H.create 37 in
  let started = H.create 37 in
  let prototype_written = H.create 37 in
  let prototype_started = H.create 37 in
  let funs = ref [] in
  let rec write_rendered (hard:bool) (key:elem_key) =
    let rec handle_deps hard key =
      let handle_dep h k =
        match k with
          | KeyTd n when h ->
            write_rendered false k;
            List.iter (write_rendered true) (H.find_all gen_ctx.hard_deps k)
          | _ -> 
            write_rendered h k
      in
      L.iter (handle_dep false) (H.find_all gen_ctx.soft_deps key);
      L.iter (handle_dep hard) (H.find_all gen_ctx.hard_deps key);
    in
    if not (H.mem written key) then
      begin
        if H.mem started key || not hard then 
          begin
            match key with
              | KeyTd td_name ->
                if (not (H.mem written key)) && H.mem started key then 
                  exitError ("ERROR circular dependency for " ^ td_name);
                      
                H.add started key ();
                handle_deps hard key;
                (* Prevent Not_found excption in hash table *)
		let outp =
      		    try H.find gen_ctx.rendered key
      		    with Not_found -> exitError( "ERROR No entry found for key " ^ elem_key_to_s key)
		in output_string chan outp;
                H.add written key ();
              | KeyO n | KeyFn n ->
                if (not (H.mem prototype_written key)) && (H.mem prototype_started key)  then
                  exitError ("ERROR: circular dependencies in prototypes involving " ^ elem_key_to_s key)
                else if not (H.mem prototype_written key) then
                  begin
                    H.add prototype_started key ();
                    H.add prototype_written key ();
                    if H.mem gen_ctx.prototypes key then
                      match key with
                        | KeyFn _ -> funs := (H.find gen_ctx.prototypes key) :: !funs
                        | _ -> output_string chan (H.find gen_ctx.prototypes key)
                    else
                      (*exitError ("ERROR: could not render prototype for: " ^ elem_key_to_s key);*)
                      (print_endline ("WARNING: could not render prototype for: " ^ elem_key_to_s key))
                  end
          end
        else 
          begin
            H.add started key ();
            handle_deps true key;
            H.add written key ();
            if H.mem gen_ctx.rendered key then
              match key with
                | KeyFn _ -> funs := (H.find gen_ctx.rendered key) :: !funs
                | _ -> output_string chan (H.find gen_ctx.rendered key)
            else
              (print_endline ("WARNING: could not render: " ^ elem_key_to_s key);
              (* This happens with external headers. Just render a placeholder*)
              output_string chan (elem_key_to_s_placeholder gen_ctx key))
          end
      end
  in
  L.iter (write_rendered true) !(gen_ctx.xform_funs);
  output_string chan !(gen_ctx.code);
  let check_prototypes key _ =
    if not (H.mem written key) then
      write_rendered true key
  in
  H.iter check_prototypes prototype_written;
  let chan_hdr = open_out "dsu.h" in
  let len = ((L.length !(gen_ctx.sizes))+1) in
  output_string chan ("#define SIZES_ARRAY_LEN "^ (string_of_int len) ^ "\n");
  output_string chan ("int sizes_array_len = "^ (string_of_int len) ^ ";\n");
  output_string chan ("int sizes_array["^ (string_of_int len) ^ "] = {");
  L.iter (output_string chan) (!(gen_ctx.sizes));
  output_string chan "-1};\n";
  output_string chan_hdr ("#ifndef _DSU_H\n#define _DSU_H\n
typedef long typ;
extern void transform_prim(void *in, void *out, typ gen); //TODO deprecated
extern void transform_fptr(void *in, void *out, typ gen); //TODO deprecated
");
  output_string chan_hdr ("enum TYPES{");
  L.iter (output_string chan_hdr) (!(gen_ctx.macrotypes));
  output_string chan_hdr "TERMINATOR}type_macros;\n";
  insert_types chan  (!(gen_ctx.macrotypes));
  insert_register_externs_cstrider chan chan_hdr (!(gen_ctx.externs_to_reg));
  insert_register_vars_cstrider chan chan_hdr; 
  output_string chan_hdr ("enum TYPES_PRIM_OUT_PLACEHOLDER{TYPE_CHAR_OUT, TYPE_SIGNED_CHAR_OUT,  TYPE_UNSIGNED_CHAR_OUT,  TYPE_CHAR_NT_OUT,  TYPE_BOOL_OUT,  TYPE_INT_OUT,  TYPE_UNSIGNED_INT_OUT,  TYPE_SHORT_OUT,  TYPE_UNSIGNED_SHORT_OUT,  TYPE_LONG_OUT,  TYPE_UNSIGNED_LONG_OUT,  TYPE_LONG_LONG_OUT,  TYPE_UNSIGNED_LONG_LONG_OUT,  TYPE_FLOAT_OUT,  TYPE_DOUBLE_OUT,  TYPE_LONG_DOUBLE_OUT,  TYPE_FUNPTR_OUT,  TYPE_OPAQUE_PTR_OUT  }type_prim_out_placeholder_macros;\n");
  output_string chan ("int main_types_array["^ (string_of_int len) ^ "] = {");
  L.iter (output_string chan) (!(gen_ctx.macrotypes_generics_maintype));
  output_string chan "-1};\n";
  output_string chan ("int target_type_array["^ (string_of_int len) ^ "] = {");
  L.iter (output_string chan) (!(gen_ctx.macrotypes_target_type));
  output_string chan "-1};\n";
  output_string chan ("int ptsto_type_array["^ (string_of_int len) ^ "] = {");
  L.iter (output_string chan) (!(gen_ctx.macrotypes_ptrtome_type));
  output_string chan "-1};\n";
  output_string chan ("static int num_gen_args_arr["^ (string_of_int len) ^ "];\n");
  output_string chan ("static typ *args_arr["^ (string_of_int len) ^ "];\n");
  output_string chan 
"int cstrider_is_prim(typ x){ //user not allowed to add addl. prim types, so existing arrays ok
   if((x==TYPE_CHAR)||(x==TYPE_SIGNED_CHAR)||(x==TYPE_UNSIGNED_CHAR)||(x==TYPE_CHAR_NT)||(x == TYPE_BOOL)||(x== TYPE_INT)||(x== TYPE_UNSIGNED_INT)||(x== TYPE_SHORT)||(x== TYPE_UNSIGNED_SHORT)||(x== TYPE_LONG)||(x== TYPE_UNSIGNED_LONG)||(x== TYPE_LONG_LONG)||(x== TYPE_UNSIGNED_LONG_LONG)||(x== TYPE_FLOAT)||(x== TYPE_DOUBLE)||(x== TYPE_LONG_DOUBLE)||(x==TYPE_OPAQUE_PTR))
   return 1; else return 0;
}\n";
  output_string chan "int cstrider_is_funptr(typ t){if((t==TYPE_FUNPTR) || (cstrider_get_tvers_funptr(t) == transform_fptr)) return 1; else return 0;}\n";
  output_string chan_hdr ("#endif\n");
  L.iter (output_string chan) (List.rev !funs); 
  output_string chan ("void (*corresp_func_ptr[])(void *arg_in,void *arg_out, typ gen)= {");
  L.iter (output_string chan) (!(gen_ctx.macrotypes_funs));
  output_string chan ( "NULL};\n");
  output_string chan ("void (*visitalls["^ (string_of_int (List.length (!(gen_ctx.visitall))+1)) ^ "]) = {");
  L.iter (output_string chan) (!(gen_ctx.visitall));
  output_string chan "NULL};\n";
  output_string chan "int visit_all_len = ";
  output_string chan (string_of_int (List.length (!(gen_ctx.visitall))));
  output_string chan ";\n";
  output_string chan 
" 
/* weirdness due to statics in generated code...but must be 
 * static b/c of stupid way declared..*/
int get_num_gen_args_arr(int i){
   return num_gen_args_arr[i];
}
typ* get_args_arr(int i){
   return args_arr[i];
}

/* returns -1 if no out type */
typ cstrider_out_type_internal(typ x){
   if(cstrider_is_prim(x))
     return -1;
   if ((x % 2) == 0) 
     return -1;
   return x+1;
}\n";
output_string chan("/*avoids error: initializer element is not constant...*/\nvoid build_gen_arrays(void){\n");
  L.iter (output_string chan) (!(gen_ctx.stored_num_gen_args));
  L.iter (output_string chan) (!(gen_ctx.stored_gen_args));
  output_string chan "\n}\n"




let generate_file (chan:out_channel) compare_ctx preamble v0_elems v1_elems results =
  let gen_ctx = gencontext_create () in
  output_string chan 
"#define E_NOANNOT
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include \"dsu.h\"
#include <cstrider_api.h>
";
  (match preamble with
    | None -> ();
    | Some code -> gen_ctx.code := code);
  L.iter (handle_elem (gencontext_set_renamer gen_ctx old_rename)) v0_elems;
  L.iter (handle_elem (gencontext_set_renamer gen_ctx new_rename)) v1_elems;
  handle_undefined_types gen_ctx;
  L.iter (grab_generic_sigs gen_ctx) results;
  L.iter (generate_trans_func gen_ctx compare_ctx) results;
  write_ordered_symbols chan gen_ctx

    
      (* cleanup *)
    
let rec check_trans mappings =
  let check_mapping = function
    | UnmatchedAdded me ->
      ["Unable to match added symbol " ^ (Xflang.path_to_string me)]
    | UnmatchedType (me0, me1) ->
      ["Unable to automatically support type change between:\n  " ^ 
          (Xflang.path_to_string me0) ^ "\nand\n  " ^ (Xflang.path_to_string me1)]
    | UnmatchedError ((me0, me1), error) -> [error]
    | MatchAuto (_, _, _, MatchStruct (_, smappings))
    | MatchAuto (_, _, _, MatchUnion smappings) ->
      check_trans smappings
    | Extern | MatchInit _ | MatchAuto _ | UnmatchedDeleted _ -> []     
  in
  L.flatten (L.map check_mapping mappings)

(*
  Code to parse the command-line options and initiate the comparison.
*)
let parseArgs () =
  match Array.to_list Sys.argv with
    | [_; out_file; v0_file; v1_file; xf_file] ->
      (out_file, v0_file, v1_file, Some xf_file)
    | [_; out_file; v0_file; v1_file] ->
      (out_file, v0_file, v1_file, None)
    | _ ->
      exitError "Please provide ktt file for each version (v0 and v1)."

let xfgen_main () =
  let (out_file, v0_file, v1_file, xf_file) = parseArgs () in
  let (preamble, rules) = Xflang.parseXFRules xf_file in
  let v0_elems, _ = loadKttData v0_file in
  let v1_elems, v1_xform_reqs = loadKttData v1_file in
  let compare_ctx = compcontext_create rules v0_elems v1_elems in
  let results = compareXformElems compare_ctx (L.map snd v1_xform_reqs) in
  match check_trans results with
    | [] -> 
      let out_chan = open_out out_file in
      generate_file out_chan compare_ctx preamble v0_elems v1_elems results;
      (* cleanup *)
      Sys.remove "externfuns.txt" ;
      Sys.remove "register_vars.txt" 
    | errors ->
      prerr_endline ("ERROR: Could not generate transformation code:");
      L.iter prerr_endline errors;
      exit 1;;

xfgen_main ()

