#include <string.h>
#include "genometools.h"
#include "VangRelation.h"
#include "VangSchemaEntry.h"

#define MAX_LINE_LENGTH 2048

//----------------------------------------------------------------------------//
// Private data structure definitions
//----------------------------------------------------------------------------//

struct VangSchemaEntry
{
  char *datatype;
  GtArray *relations;
  GtArray *relation_exclusions;
  GtArray *required_attributes;
};

struct VangRelationExclusion
{
  GtArray *exclusive_relations;
  char *note;
};


//----------------------------------------------------------------------------//
// Prototypes for private methods
//----------------------------------------------------------------------------//

/**
 * Parse a relation object from its string representation
 *
 * @param[in] relstr    string representation of a relation
 * @returns             a relation object
 */
VangRelation *vang_schema_entry_parse_relation(char *relstr);

/**
 * Parse an exclusion object from its string representation
 *
 * @param[in] exclstr    string representation of an exclusion
 * @returns              an exclusion object
 */
VangRelationExclusion *vang_schema_entry_parse_exclusion(char *exclstr);

/**
 * Parse a list of required attributes from the corresponding string
 * representation
 *
 * @param[in] attrstr    string representation of a list of required attributes
 * @returns              an array containing the attribute keys
 */
GtArray *vang_schema_entry_parse_attributes(char *attrstr);

/**
 * Allocate memory for a new schema entry object
 *
 * @param[in] datatype    data type corresponding to this entry
 * @returns               an entry object containing all relations, exclusions,
 *                        and required attributes associated with the associated
 *                        data type
 */
VangSchemaEntry *vang_schema_entry_new(const char *datatype);

/**
 * Associate a relation object with the given entry
 *
 * @param[in] entry       an entry
 * @param[in] relation    relation to be associated with the entry
 */
void vang_schema_entry_add_relation( VangSchemaEntry *entry,
                                     VangRelation *relation );

/**
 * Associate an exclusion object with the given entry
 *
 * @param[in] entry        an entry
 * @param[in] exclusion    exclusions to be associated with the entry
 */
void vang_schema_entry_add_exclusion( VangSchemaEntry *entry,
                                      VangRelationExclusion *exclusion );

/**
 * Associate a list of required attributes with the given entry
 *
 * @param[in] entry         an entry
 * @param[in] attributes    list of required attributes to be associated with
 *                          the entry
 */
void vang_schema_entry_add_attributes( VangSchemaEntry *entry,
                                       GtArray *attributes );

/**
 * Allocate memory for a relation exclusion object
 *
 * @returns    a relation exclusion object
 */
VangRelationExclusion *vang_relation_exclusion_new();

/**
 * Associate a relation (by ID) with this exclusion
 *
 * @param[in] exclusion    an exclusion object
 * @param[in] relid        ID of the relation to be associated with this
 *                         exclusion
 */
void vang_relation_exclusion_add_relation( VangRelationExclusion *exclusion,
                                           char *relid );

/**
 * Add a descriptive free-text note to this relation exclusion
 *
 * @param[in] exclusion    an exclusion object
 * @param[in] note         text of the note to be stored
 */
void vang_relation_exclusion_set_note( VangRelationExclusion *exclusion,
                                       const char *note );

/**
 * Print a string representation of this relation exclusion
 *
 * @param[in]  exclusion    exclusion object
 * @param[out] outstream    output stream to which data will be printed
 */
void vang_relation_exclusion_to_string( VangRelationExclusion *exclusion,
                                        FILE *outstream );

//----------------------------------------------------------------------------//
// Public method implementations
//----------------------------------------------------------------------------//

VangSchemaEntry *vang_schema_entry_next(FILE *schemafile)
{
  char buffer[MAX_LINE_LENGTH];
  VangSchemaEntry *entry = NULL;

  while( fgets(buffer, MAX_LINE_LENGTH, schemafile) != NULL )
  {
    if(buffer[strlen(buffer)-1] == '\n')
      buffer[strlen(buffer)-1] = '\0';
    if(buffer[0] == '#' || strcmp(buffer, "") == 0)
      continue;

    GtArray *tokens = gt_array_new( sizeof(char *) );
    char *tok = strtok(buffer, "\t");
    char *tokcpy = gt_cstr_dup(tok);
    gt_array_add(tokens, tokcpy);
    while((tok = strtok(NULL, "\t")) != NULL)
    {
      tokcpy = gt_cstr_dup(tok);
      gt_array_add(tokens, tokcpy);
    }

    char *entrytype = *(char **)gt_array_get(tokens, 0);
    entry = vang_schema_entry_new(entrytype);

    unsigned long i;
    for(i = 1; i < gt_array_size(tokens); i++)
    {
      char *token = *(char **)gt_array_get(tokens, i);
      if(strncmp(token, "Relation=", strlen("Relation=")) == 0)
      {
        VangRelation *relation = vang_schema_entry_parse_relation(token);
        vang_schema_entry_add_relation(entry, relation);
      }
      else if(strncmp(token, "Exclusive=", strlen("Exclusive=")) == 0)
      {
        VangRelationExclusion *excl = vang_schema_entry_parse_exclusion(token);
        vang_schema_entry_add_exclusion(entry, excl);
      }
      else if(strncmp(token, "Attribute=", strlen("Attribute=")) == 0)
      {
        GtArray *attributes = vang_schema_entry_parse_attributes(token);
        vang_schema_entry_add_attributes(entry, attributes);
        gt_array_delete(attributes);
      }
      else
      {
        fprintf(stderr, "error: unsupported token '%s'", token);
        exit(1);
      }
      gt_free(token);
    }
    gt_free(entrytype);
    gt_array_delete(tokens);

    break;
  }

  return entry;
}

void vang_relation_exclusion_delete(VangRelationExclusion *exclusion)
{
  unsigned long i;

  for(i = 0; i < gt_array_size(exclusion->exclusive_relations); i++)
  {
    char *relid = *(char **)gt_array_get(exclusion->exclusive_relations, i);
    gt_free(relid);
  }
  gt_array_delete(exclusion->exclusive_relations);

  if(exclusion->note != NULL)
    gt_free(exclusion->note);

  gt_free(exclusion);
  exclusion = NULL;
}

const char *vang_schema_entry_get_datatype(VangSchemaEntry *entry)
{
  return entry->datatype;
}

void vang_schema_entry_to_string(VangSchemaEntry *entry, FILE *outstream)
{
  fputs(entry->datatype, outstream);

  unsigned long i;
  for(i = 0; i < gt_array_size(entry->relations); i++)
  {
    VangRelation *rel = *(VangRelation **)gt_array_get(entry->relations, i);
    fputc('\t', outstream);
    vang_relation_to_string(rel, outstream);
  }

  for(i = 0; i < gt_array_size(entry->relation_exclusions); i++)
  {
    VangRelationExclusion *excl = *(VangRelationExclusion **)
                                  gt_array_get(entry->relation_exclusions, i);
    fputc('\t', outstream);
    vang_relation_exclusion_to_string(excl, outstream);
  }

  if(gt_array_size(entry->required_attributes) > 0)
  {
    fputs("\tAttribute=", outstream);
    for(i = 0; i < gt_array_size(entry->required_attributes); i++)
    {
      const char *attr = *(const char **)
                         gt_array_get(entry->required_attributes, i);
      if(i > 0)
        fputc(',', outstream);
      fputs(attr, outstream);
    }
  }
}

void vang_schema_entry_delete(VangSchemaEntry *entry)
{
  unsigned int i;

  gt_free(entry->datatype);

  for(i = 0; i < gt_array_size(entry->relations); i++)
  {
    VangRelation *rel = *(VangRelation **)gt_array_get(entry->relations, i);
    vang_relation_delete(rel);
  }
  gt_array_delete(entry->relations);

  for(i = 0; i < gt_array_size(entry->relation_exclusions); i++)
  {
    VangRelationExclusion *excl = *(VangRelationExclusion **)
                                  gt_array_get(entry->relation_exclusions, i);
    vang_relation_exclusion_delete(excl);
  }
  gt_array_delete(entry->relation_exclusions);

  for(i = 0; i < gt_array_size(entry->required_attributes); i++)
  {
    char *attribute = *(char **)gt_array_get(entry->required_attributes, i);
    gt_free(attribute);
  }
  gt_array_delete(entry->required_attributes);

  gt_free(entry);
  entry = NULL;
}


//----------------------------------------------------------------------------//
// Private method implementations
//----------------------------------------------------------------------------//

VangRelation *vang_schema_entry_parse_relation(char *relstr)
{
  unsigned long i;
  char *tok;
  char *relstrcpy = gt_cstr_dup(relstr);
  GtArray *tokens = gt_array_new( sizeof(char *) );

  // Store all key/value token in an array
  tok = strtok(relstrcpy, ";");
  gt_array_add(tokens, tok);
  while((tok = strtok(NULL, ";")) != NULL)
  {
    gt_array_add(tokens, tok);
  }

  // Parse key/value pairs
  GtHashmap *attributes = gt_hashmap_new( GT_HASH_STRING, (GtFree)gt_free_func,
                                          (GtFree)gt_free_func );
  GtArray *mapkeys = gt_array_new( sizeof(char *) );
  for(i = 0; i < gt_array_size(tokens); i++)
  {
    char *key, *value;
    tok = *(char **)gt_array_get(tokens, i);
    key = strtok(tok, "=");
    key = gt_cstr_dup(key);
    value = strtok(NULL, "=");
    value = gt_cstr_dup(value);

    gt_hashmap_add(attributes, key, value);
    gt_array_add(mapkeys, key);
  }

  // Grab required attributes: relation ID and node type
  const char *id = gt_hashmap_get(attributes, "Relation");
  const char *nodetype = gt_hashmap_get(attributes, "Nodetype");
  if(id == NULL || nodetype == NULL)
  {
    fprintf( stderr, "error: relation ID and node type must be specified for "
             "each relation: '%s'\n", relstr );
    exit(1);
  }

  // Set any other values
  VangRelation *rel = vang_relation_new(id, nodetype);
  for(i = 0; i < gt_array_size(mapkeys); i++)
  {
    const char *key = *(char **)gt_array_get(mapkeys, i);
    if(strcmp(key, "Relation") == 0 || strcmp(key, "Nodetype") == 0)
      continue;

    const char *value = gt_hashmap_get(attributes, key);
    if(strcmp(key, "Degree") == 0)
    {
      DegreeConstraint dc = vang_degree_constraint_parse(value);
      vang_relation_set_degree(rel, dc.context, dc.operator, dc.degree);
    }
    else if(strcmp(key, "Key") == 0)
    {
      vang_relation_set_key(rel, value);
    }
    if(strcmp(key, "Spatial") == 0)
    {
      SpatialConstraint constraint = vang_spatial_constraint_parse(value);
      vang_relation_set_spatial(rel, constraint);
    }
  }

  gt_free(relstrcpy);
  gt_array_delete(tokens);
  gt_array_delete(mapkeys);
  gt_hashmap_delete(attributes);

  return rel;
}

VangRelationExclusion *vang_schema_entry_parse_exclusion(char *exclstr)
{
  char *exclstrcpy = gt_cstr_dup(exclstr);
  char *excltok = strtok(exclstrcpy, ";");
  char *notetok = strtok(NULL, ";");

  VangRelationExclusion *exclusion = vang_relation_exclusion_new();

  char *tok = strtok(excltok, "=");
  tok = strtok(NULL, "=");
  char *rid = strtok(tok, ",");
  vang_relation_exclusion_add_relation(exclusion, rid);
  while( (rid = strtok(NULL, ",")) != NULL )
  {
    vang_relation_exclusion_add_relation(exclusion, rid);
  }

  if(notetok != NULL)
  {
    vang_relation_exclusion_set_note(exclusion, notetok + strlen("Note="));
  }

  gt_free(exclstrcpy);
  return exclusion;
}

GtArray *vang_schema_entry_parse_attributes(char *attrstr)
{
  char *attrstrcpy = gt_cstr_dup(attrstr);
  GtArray *attributes = gt_array_new( sizeof(char *) );

  char *tok = strtok(attrstrcpy + strlen("Attribute="), ",");
  char *tokcpy = gt_cstr_dup(tok);
  gt_array_add(attributes, tokcpy);
  while( (tok = strtok(NULL, ",")) != NULL )
  {
    tokcpy = gt_cstr_dup(tok);
    gt_array_add(attributes, tokcpy);
  }

  gt_free(attrstrcpy);

  return attributes;
}

VangSchemaEntry *vang_schema_entry_new(const char *datatype)
{
  VangSchemaEntry *entry = gt_malloc( sizeof(VangSchemaEntry) );

  entry->datatype = gt_cstr_dup(datatype);
  entry->relations = gt_array_new( sizeof(VangRelation *) );
  entry->relation_exclusions = gt_array_new( sizeof(VangRelationExclusion *) );
  entry->required_attributes = gt_array_new( sizeof(char *) );

  return entry;
}

void vang_schema_entry_add_relation( VangSchemaEntry *entry,
                                     VangRelation *relation )
{
  gt_array_add(entry->relations, relation);
}

void vang_schema_entry_add_exclusion( VangSchemaEntry *entry,
                                      VangRelationExclusion *exclusion )
{
  gt_array_add(entry->relation_exclusions, exclusion);
}

void vang_schema_entry_add_attributes( VangSchemaEntry *entry,
                                       GtArray *attributes )
{
  gt_array_add_array(entry->required_attributes, attributes);
}

VangRelationExclusion *vang_relation_exclusion_new()
{
  VangRelationExclusion *exclusion = gt_malloc( sizeof(VangRelationExclusion) );
  exclusion->exclusive_relations = gt_array_new( sizeof(char *) );
  exclusion->note = NULL;

  return exclusion;
}

void vang_relation_exclusion_add_relation( VangRelationExclusion *exclusion,
                                           char *relid )
{
  char *relidcpy = gt_cstr_dup(relid);
  gt_array_add(exclusion->exclusive_relations, relidcpy);
}

void vang_relation_exclusion_set_note( VangRelationExclusion *exclusion,
                                       const char *note )
{
  if(exclusion->note != NULL)
    gt_free(exclusion->note);
  exclusion->note = gt_cstr_dup(note);
}

void vang_relation_exclusion_to_string( VangRelationExclusion *exclusion,
                                        FILE *outstream )
{
  fputs("Exclusive=", outstream);

  unsigned long i;
  for(i = 0; i < gt_array_size(exclusion->exclusive_relations); i++)
  {
    const char *relid = *(const char **)
                        gt_array_get(exclusion->exclusive_relations, i);
    if(i > 0)
      fputc(',', outstream);
    fputs(relid, outstream);
  }

  if(exclusion->note != NULL)
  {
    fprintf(outstream, ";Note=%s", exclusion->note);
  }
}
