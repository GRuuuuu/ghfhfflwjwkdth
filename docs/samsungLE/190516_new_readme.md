# LE Custom Chart Demo Test

> Last Test env Ver.  
>`logstash` v5.4.1  
>`elasticsearch` v6.7.1  

## 테스트 목적 
LSF에는 `user`에 대한 `department`정보가 존재하지 않음. `department`정보를 `user`의 dependency로 묶어 LE에서 차트로 확인할 수 있게 한다.  
  
## STEP 1. Template & Logstash
먼저 `user_name`에 대한 `department_name`정보를 es의 index로 만들어 두기 위해 각 정보를 기재한 csv파일을 Logstash의 input으로 넣는다.  

Logstash의 conf파일은 다음과 같다.  

**user_map.conf**
~~~bash
input {
    file {
            path => "/root/q/testdata.csv"
            sincedb_path => "/root/q/.opsition"
            start_position => "beginning"
        }
}

filter {
    csv {
        columns => ["department_name", "project_name", "user_name"]
        remove_field => ["message", "path", "host", "@version"]
    }
     if [user_name] == "user" {drop { }}
}

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

Parsing하고나서 만들어진 doc는 elasticsearch에 들어가야 된다.  
output에 elasticsearch를 정의해주고 doc가 어떠한 형식으로 들어갈 것인지 template를 정의해 주어야 한다.  

template는 json파일로 구성되어있다.  

**template.json**
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
>다음 커맨드를 실행하기 전에  
>1. sys_user_mapping 인덱스 삭제
>2. sys_user_mapping 템플릿 삭제
>3. .opsition 파일 삭제  
>를 진행한 후에 실행해야함.  

conf file을 실행시키는 커맨드는 다음과 같다.  
~~~bash
$ {logstash_dir}/bin/logstash -f user_map.conf
~~~  

## STEP 2. Update DataLoader
`user_name`과 `department_name`에 관한 index `sys_user_mapping`이 elasticsearch에 doc로 생성되었으므로 실제 LSF에서 생성되는 데이터에 department정보가 들어가야 한다.  

LSF에서 데이터가 생성되어 elasticsearch에 저장될 때 dataloader를 통해 필요한 항목만 index의 doc로 생성이 된다.  

즉 `user_name`의 dependency로 `sys_user_mapping` 인덱스에서 `department_name`정보를 가져오게 하면 된다.  

이게 가능하려면 dataloader 정의 xml 파일에 Key로 정의한 필드(`user_name`)가 PK로 정의되어 있어야 하고, 추가할 외부 필드는 <Extra ...> 양식으로 정의해주면 된다. 

dataloader들이 담긴 폴더는 다음 위치에 있다.
~~~bash
/opt/ibm/lsfsuite/ext/perf/conf/dataloader
~~~

**lsbacct.xml**
~~~xml
<!--수정한 부분만 기재-->
...
                        <Field Name="user_name" Column="USER_NAME" />
                        <!-- 기본 필드 정의에 user_name은 포함되어 있으나, department_name은 없음-->
...
                        <PK Name="cluster_name"/>
                        <PK Name="event_type"/>
                        <PK Name="job_id"/>
                        <PK Name="job_arr_idx"/>
                        <PK Name="event_time_utc"/>
                        <PK Name="user_name"/> <!-- user_name을 Primary Key로 선언--> 
                        <Extra Name="department_name"/>	 	
                        <!-- 기본 필드에 없고, 아래 Dependency 구문을 통해 
                        가져올 department_name을 Extra Name으로 선언-->
                </SQL>
.....
                </Mappings>
                <Dependencies>	<!-- 다른 인덱스 데이터를 정의하기 위한 시작-->
                        <Dependency Name="sys_user_mapping">	
                        <!-- 참조할 Elasticsearch index 이름 명시 
                        (이 인덱스에는 Key 과 Value가 1:1로 정의되어 있어야 함)-->
                                <Keys>
                                        <!-- Key 가 되는 필드명 명시-->
                                        <Key Name="user_name"/>	
                                </Keys>
                                <Values>
                                        <!-- 가져올 필드명 명시-->
                                        <Value Name="department_name"/>	
                                </Values>
                        </Dependency>
                </Dependencies>
        </Writer>
</DataLoader>
~~~

>이미 doc가 쌓인 상태에서 dataloader를 수정하게 되면 몇가지 문제가 발생할 수 있다.  
>때문에 기존의 doc들을 수정하거나 index mapping구조를 변경해주어야한다.  
>아직 충분한 테스트가 이뤄지지 않았으므로 추후에 다시 서술 예정  
>일단은 기존에 쌓인 인덱스들을 날리고 reconfig를 하도록 한다.  
>~~~bash
>$ curl -XDELETE {ip}:9200/{index name}?pretty
>~~~

xml파일을 수정한 후에 reconfig를 해야한다.  
~~~bash
$ perfadmin stop all
$ perfadmin start all
~~~  

## STEP 3. Report customization description
`department_name`이 포함된 doc들이 구성완료가 되면 chart를 생성할때 department_name 필드도 추가되어야 chart에서 확인할 수 있다.  
es의 모델을 update해주는 과정이다.  

~~~bash
$ ./init_report_model.sh
~~~

차트를 새로 만든 결과  
![image](https://user-images.githubusercontent.com/15958325/57895777-6c566f00-7888-11e9-81a3-30bbb9db0f4f.png) 

![image](https://user-images.githubusercontent.com/15958325/57895704-05d15100-7888-11e9-8603-d041d75f983c.png)  

![image](https://user-images.githubusercontent.com/15958325/57895705-0964d800-7888-11e9-8389-a30a9626582e.png)
