#include "add-contextual-data-template-selector.h"
#include "template/templates.h"
#include "syslog-ng.h"
#include "messages.h"

typedef struct _AddContextualDataTemplateSelector
{
  AddContextualDataSelector super;
  gchar *selector_template_string;
  LogTemplate *selector_template;
} AddContextualDataTemplateSelector;

static gboolean
_compile_selector_template(AddContextualDataTemplateSelector *self)
{
  GError *error = NULL;
  if (!self->selector_template_string)
    {
      msg_error("No selector set.");
      return FALSE;
    }

  if (!log_template_compile(self->selector_template, self->selector_template_string, &error))
    {
      msg_error("Failed to compile template",
                evt_tag_str("template", self->selector_template_string),
                evt_tag_str("error", error->message));
      return FALSE;
    }
 
  return TRUE;
}

static void
_replace_template(LogTemplate **old_template, LogTemplate *new_template)
{
  log_template_unref(*old_template);
  *old_template = log_template_ref(new_template);
}

static gboolean
_init(AddContextualDataSelector *s)
{
  AddContextualDataTemplateSelector *self = (AddContextualDataTemplateSelector *)s;
  return _compile_selector_template(self);
}

static gchar*
_resolve(AddContextualDataSelector *s, LogMessage *msg)
{
  GString *selector_str = g_string_new(NULL);  
  AddContextualDataTemplateSelector *self = (AddContextualDataTemplateSelector *)s;

  log_template_format(self->selector_template, msg, NULL, LTZ_LOCAL, 0, NULL,
                        selector_str);

  return g_string_free(selector_str, FALSE);
}

static void
_free(AddContextualDataSelector *s)
{
  AddContextualDataTemplateSelector *self = (AddContextualDataTemplateSelector *)s;
  log_template_unref(self->selector_template);
  g_free(self->selector_template_string);
}

static AddContextualDataSelector *
_clone(AddContextualDataSelector *s, GlobalConfig *cfg)
{
  AddContextualDataTemplateSelector *self = (AddContextualDataTemplateSelector *)s;
  AddContextualDataTemplateSelector *cloned = (AddContextualDataTemplateSelector *)add_contextual_data_template_selector_new(cfg, self->selector_template_string);
  _replace_template(&cloned->selector_template, self->selector_template);

  return &cloned->super;
}

AddContextualDataSelector*
add_contextual_data_template_selector_new(GlobalConfig *cfg, const gchar *selector_template_string)
{
  AddContextualDataTemplateSelector *new_instance = g_new0(AddContextualDataTemplateSelector, 1);
  new_instance->selector_template_string = g_strdup(selector_template_string);
  new_instance->selector_template = log_template_new(cfg, NULL);
  new_instance->super.resolve = _resolve;
  new_instance->super.free = _free;
  new_instance->super.init = _init;
  new_instance->super.clone = _clone;

  return &new_instance->super;
}
