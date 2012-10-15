import sys
import pg_logger
import json


def json_finalizer(input_code, output_trace):
  ret = dict(code=input_code, trace=output_trace)
  json_output = json.dumps(ret, indent=None) # use indent=None for most compact repr
  print(json_output)


fin = open(sys.argv[1])

pg_logger.exec_script_str(fin.read(), False, json_finalizer)
