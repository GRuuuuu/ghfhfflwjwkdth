# Error

>수정코드 : [Box Link](https://ibm.ent.box.com/folder/76356961446)

## 1. init_report_model.sh in es 6.x

~~~bash
$ sh init_report_model.sh
{"error":{"root_cause":[{"type":"illegal_argument_exception","reason":"Rejecting mapping update to [model] as the final mapping would have more than 1 type: [cubes, doc]"}],"type":"illegal_argument_exception","reason":"Rejecting mapping update to [model] as the final mapping would have more than 1 type: [cubes, doc]"},"status":400}
{"error":{"root_cause":[{"type":"illegal_argument_exception","reason":"Rejecting mapping update to [model] as the final mapping would have more than 1 type: [cubes, doc]"}],"type":"illegal_argument_exception","reason":"Rejecting mapping update to [model] as the final mapping would have more than 1 type: [visualizations, doc]"},"status":400}
...
~~~  
위와같은 에러가 발생.  

### 원인
기존에 es 5.x버전에서 돌아가던 코드는 es 6.x에서 실행되지 않음.  

이유는 6.x로 올라오면서 `index`가 `multiple mapping type`을 지원하지 않기 때문.    
참고 : [breaking-changes-6.0](https://www.elastic.co/guide/en/elasticsearch/reference/6.0/breaking-changes-6.0.html)   

### 해결

~~~json
curl -XPUT "http://${es_url}/model/cubes/zkexa_visual_1547635002619_cube" -H 'Content-Type: application/json' -d'
{
  
    "id": "zkexa_visual_1547635002619_cube",
    "metadata": {
      "version": "10.1"
    },
    ...
}
~~~

to

~~~json
curl -XPUT "http://${es_url}/model/doc/zkexa_visual_1547635002619_cube" -H 'Content-Type: application/json' -d'
{
  
    "id": "zkexa_visual_1547635002619_cube",
    "metadata": {
      "version": "10.1"
    },
    "type": "cube",
    ...
}
~~~

>기존에 왔던 중국 코드를 그대로 사용하면 됨.

## 2. logstash can't store data in es

기존의 user_mapping.conf를 돌렸을 때 문제를 총 세가지 발견.

### 2.1 using default template
~~~bash
[INFO ][logstash.outputs.elasticsearch] Using default mapping template
[INFO ][logstash.outputs.elasticsearch] Attempting to install template {:manage_template=>{"template"=>"logstash-*", "version"=>60001, "settings"=>{"index.refresh_interval"=>"5s"}, "mappings"=>{"_default_"=>{"dynamic_templates"=>[{"message_field"=>{"path_match"=>"message", "match_mapping_type"=>"string", "mapping"=>{"type"=>"text", "norms"=>false}}}, {"string_fields"=>{"match"=>"*", "match_mapping_type"=>"string", "mapping"=>{"type"=>"text", "norms"=>false, "fields"=>{"keyword" =>{"type"=>"keyword", "ignore_above"=>256}}}}}], "properties"=>{"@timestamp"=>{"type"=>"date"}, "@version"=>{"type"=>"keyword"}, "geoip"=>{"dynamic"=>true, "properties"=>{"ip"=>{"type"=>"ip"}, "location"=>{"type"=>"geo_point"}, "latitude"=>{"type"=>"half_float"}, "longitude"=>{"type"=>"half_float"}}}}}}}}
~~~
readme 1번에서 생성했던 템플릿인 sys_user_mapping을 사용하지 않고, default mapping template를 사용하고 있다.  

**해결**  
logstash의 conf파일을 수정해주어야한다.  

~~~json
output {
    stdout { codec => json }
    elasticsearch {
        hosts => ["localhost:9200"]
        index => "sys_user_mapping"
        document_id => "%{user_name}"
        manage_template => true
        template => "./template.json"
        template_name => "sys_user_mapping"
        template_overwrite => true
     }
}
~~~
어떤 template를 사용할건지 지정해주어야한다.  

실행시키면 default가아니라 만들었던 template를 사용한다는것을 확인할 수 있다.  
~~~bash
[INFO ][logstash.outputs.elasticsearch] Using mapping template from {:path=>"./template.json"}
[INFO ][logstash.outputs.elasticsearch] Attempting to install template {:manage_template=>{"template"=>"sys_user_mapping", "order"=>1, "settings"=>{"number_of_shards"=>"1", "number_of_replicas"=>"1"}, "mappings"=>{"_default_"=>{"_all"=>{"enabled"=>false}, "date_detection"=>true, "numeric_detection"=>true, "dynamic_templates"=>[{"str"=>{"match"=>"*", "match_mapping_type"=>"string", "mapping"=>{"type"=>"text", "fields"=>{"raw"=>{"type"=>"keyword", "ignore_above"=>256}}}}}]}}}}
~~~

### 2.2 csv Parsing Error
~~~bash
Error parsing csv {:field=>"message", :source=>"", :exception=>#<NoMethodError: undefined method `each_index' for nil:NilClass>}
{"@timestamp":"2019-05-12T07:10:22.841Z",  ...
~~~

**해결**  
csv파일에 blank line이 있기 때문. 해당 line을 지워주면 해결된다.    


### 2.3 could not index event to es
~~~bash
Could not index event to Elasticsearch. {:status=>400, :action=>["index", {:_id=>"panpan", :_index=>"sys_user_mapping", :_type=>"doc", :routing=>nil}, #<LogStash::Event:0x1f14851a>], :response=>{"index"=>{"_index"=>"sys_user_mapping", "_type"=>"doc", "_id"=>"panpan", "status"=>400, "error"=>{"type"=>"mapper_parsing_exception", "reason"=>"failed to find type parsed [string] for [user_name]"}}}}
...
~~~  

*이전의 템플릿
~~~json
{
    "template": "sys_user_mapping",
    "order": 1,
    "settings":{
        "number_of_shards": "'${PRI_SHARD_NUM}'",
        "number_of_replicas": "'${REP_SHARD_NUM}'"
    },
    "mappings": {
        "_default_": {
            "_all":{"enabled": false},
            "date_detection" : true,
            "numeric_detection" : true,
            "dynamic_templates":[
                {
                    "str":{
                        "match": "*",
                        "match_mapping_type": "string",
                        "mapping":{
                            "type": "string",
                            "index" : "not_analyzed"
                        }
                    }
                }
            ]
        }
    }
}
~~~


**해결**  
template에서 mapping하는부분 + deprecate된 항목 수정

~~~json
{
    "template": "sys_user_mapping",
    "order": 1,
    "settings":{
        "number_of_shards": "1",
        "number_of_replicas": "1"
        },
    "mappings": {
        "doc": {
            "date_detection" : true,
            "numeric_detection" : true,
            "dynamic_templates":[{
                "str":{
                    "match_mapping_type": "string",
                    "mapping":{
                        "type":"text",
                        "fields":{
                            "raw":{
                                "type":"keyword",
                                "ignore_above":256
                            }
                        }
                    }
                }
            }]
        }
    }
}
~~~
`_default_`필드는 6.0부터 deprecated될거고 사라질 항목이어서 type name을 입력.   
`_all`필드도 사라질예정. default가 disable이어서 없애도 문제는 없다.    
`string` -> `text`  
`"index" : "not_analyzed"` -> `"type":"keyword"`  

참고: [Dynamic Templates](https://www.elastic.co/guide/en/elasticsearch/reference/current/dynamic-templates.html#match-unmatch)  


index에 제대로 들어간 모습
![image](https://user-images.githubusercontent.com/15958325/57582284-aaa50480-74fd-11e9-9343-f944e2ffa023.png)  

![image](https://user-images.githubusercontent.com/15958325/57582287-babce400-74fd-11e9-91af-5ccaeb22df94.png)  


template적용된 모습
![image](https://user-images.githubusercontent.com/15958325/57582300-dc1dd000-74fd-11e9-9a9f-20f9b4bf0adb.png)  


