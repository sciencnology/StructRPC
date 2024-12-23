from jinja2 import Template

class MemberFunction:
    def __init__(self, name):
        self.name = name
        self.return_type = 'void'
        self.params = []

    def addReturnType(self, return_type, is_const=False, is_reference=False):
        self.return_type = {
            'type': return_type,
            'is_const': is_const,
            'is_reference': is_reference
        }

    def addParam(self, param_type, param_name, is_const=False, is_reference=False, class_name=None, key_type=None, value_type=None):
        self.params.append({
            'type': param_type,
            'name': param_name,
            'is_const': is_const,
            'is_reference': is_reference,
            'className': class_name,
            'keyType': key_type,
            'valueType': value_type
        })

    def generate_param_class(self, generator):
        param_class_name = f"{generator.class_name}_{self.name}_Params"
        class_def = f"class {param_class_name}\n{{\npublic:\n"
        for param in self.params:
            type_str = generator.translate_type(param['type'], param.get('className'), param.get('keyType'), param.get('valueType'), param.get('is_const'), param.get('is_reference'))
            class_def += f"    {type_str} {param['name']};\n"
        if self.params:
            class_def += f"    NLOHMANN_DEFINE_TYPE_INTRUSIVE({param_class_name}, {', '.join(p['name'] for p in self.params)});\n"
        else:
            class_def += f"    NLOHMANN_DEFINE_TYPE_INTRUSIVE({param_class_name});\n"
        class_def += "};\n\n"
        return class_def



class CppClassGenerator:
    def __init__(self):
        self.class_name = ""
        self.member_variables = []
        self.member_functions = []

    def setClassName(self, class_name):
        self.class_name = class_name

    def addMemberVariable(self, var_type, var_name, class_name=None, key_type=None, value_type=None):
        type_str = self.translate_type(var_type, class_name, key_type, value_type)
        self.member_variables.append({'type': type_str, 'name': var_name})

    def addMemberFunction(self, member_function):
        translated_params = [
            {
                'type': self.translate_type(p['type'], p.get('className'), p.get('keyType'), p.get('valueType'), p.get('is_const'), p.get('is_reference')),
                'name': p['name']
            }
            for p in member_function.params
        ]
        return_type_str = self.translate_type(member_function.return_type['type'], is_const=member_function.return_type['is_const'], is_reference=member_function.return_type['is_reference'])
        self.member_functions.append({
            'return_type': return_type_str,
            'name': member_function.name,
            'params': translated_params,
            'param_class': member_function.generate_param_class(self)
        })

    def translate_type(self, var_type, class_name=None, key_type=None, value_type=None, is_const=False, is_reference=False):
        if var_type == 'list':
            type_str = f'std::vector<{class_name}>'
        elif var_type == 'map':
            key_str = self.translate_type(key_type)
            value_str = self.translate_type(value_type)
            type_str = f'std::map<{key_str}, {value_str}>'
        else:
            type_str = var_type
        
        if is_const:
            type_str = f'const {type_str}'
        if is_reference:
            type_str = f'{type_str}&'

        return type_str

    def generate(self):
        template = Template('''
#include <nlohmann/json.hpp>

class {{ class_name }} 
{
public:
    {% for var in member_variables %}
    {{ var.type }} {{ var.name }};
    {% endfor %}
    
    {% for func in member_functions %}
    {{ func.return_type }} {{ func.name }}({% for param in func.params %}{{ param.type }} {{ param.name }}{% if not loop.last %}, {% endif %}{% endfor %});
    {% endfor %}
};

{% for func in member_functions %}
{{ func.param_class }}
{% endfor %}

class {{ class_name }}Wrapper 
{
private:
    {{ class_name }} instance;
public:
    {% for func in member_functions %}
    nlohmann::json {{ func.name }}(const nlohmann::json& j) 
    {
        auto params = j.get<{{ class_name }}_{{ func.name }}_Params>();
        {% if func.return_type != 'void' %}
        auto result = instance.{{ func.name }}({% for param in func.params %}params.{{ param.name }}{% if not loop.last %}, {% endif %}{% endfor %});
        return result;
        {% else %}
        instance.{{ func.name }}({% for param in func.params %}params.{{ param.name }}{% if not loop.last %}, {% endif %}{% endfor %});
        return nlohmann::json{};
        {% endif %}
    }
    {% endfor %}
};
        ''')
        return template.render(
            class_name=self.class_name,
            member_variables=self.member_variables,
            member_functions=self.member_functions

        )

# Example usage
generator = CppClassGenerator()
generator.setClassName("MyClass")
generator.addMemberVariable("int", "member1")
generator.addMemberVariable("std::string", "member2")
generator.addMemberVariable("list", "memberList", class_name="MyType")
generator.addMemberVariable("map", "memberMap", key_type="std::string", value_type="int")

func1 = MemberFunction("setMember1")
func1.addReturnType("void")
func1.addParam("int", "value", is_const=True)
func1.addParam("list", "value2", class_name="MyType", is_const=True)

func2 = MemberFunction("getMember1")
func2.addReturnType("int", is_reference=True)

generator.addMemberFunction(func1)
generator.addMemberFunction(func2)

cpp_code = generator.generate()
print(cpp_code)
