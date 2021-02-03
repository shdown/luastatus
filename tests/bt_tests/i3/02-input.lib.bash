x_testcase_input '{"name":"0","foo":"bar"}' '{["foo"]="bar",["name"]="0"}'
x_testcase_input '{"name":"0","foo":[1,2,3,{"k":"v"},4]}' '{["foo"]={1.0,2.0,3.0,{["k"]="v"},4.0},["name"]="0"}'
x_testcase_input '{"name":"0","foo":null,"bar":true,"baz":false}' '{["bar"]=true,["baz"]=false,["name"]="0"}'
