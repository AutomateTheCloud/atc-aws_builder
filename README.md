# AWS Builder
- Tools to assist in making AWS Infrastructure easier to manage, more robust, and more secure.
- Documentation Updated: 2019-10-04 21:06:49 UTC

## Table of Contents
   * [Getting Started](#getting-started)
      * [Prerequisites](#prerequisites)
      * [Installing](#installing)
   * [Scripts](#scripts)
      * [ami](#ami)
         * [amazon_linux2_encrypted.sh](#amazon_linux2_encryptedsh)
      * [cloudformation](#cloudformation)
         * [build.sh](#buildsh)
         * [deploy.sh](#deploysh)
         * [process.sh](#processsh)
         * [targets.sh](#targetssh)
         * [validate.sh](#validatesh)
      * [cloudfront](#cloudfront)
         * [invalidate_cache.sh](#invalidate_cachesh)
      * [codedeploy](#codedeploy)
         * [deploy_revision.sh](#deploy_revisionsh)
         * [register_revision.sh](#register_revisionsh)
      * [ec2](#ec2)
         * [generate_ssh_keypair.sh](#generate_ssh_keypairsh)
      * [elasticache/redis](#elasticacheredis)
         * [create.sh](#createsh)
      * [kms](#kms)
         * [decrypt_file.sh](#decrypt_filesh)
         * [encrypt_file.sh](#encrypt_filesh)
      * [rds/aurorapostgresql](#rdsaurorapostgresql)
         * [create.sh](#createsh)
      * [rds/mysql](#rdsmysql)
         * [create.sh](#createsh)
      * [rds/postgresql](#rdspostgresql)
         * [create.sh](#createsh)
      * [rds/tools](#rdstools)
         * [download_logs.sh](#download_logssh)
         * [sync_all_logs_to_s3.sh](#sync_all_logs_to_s3sh)
      * [s3](#s3)
         * [create_temp_bucket.sh](#create_temp_bucketsh)
         * [download_file.sh](#download_filesh)
         * [move_file.sh](#move_filesh)
         * [upload_file.sh](#upload_filesh)
         * [upload_website.sh](#upload_websitesh)
      * [sam](#sam)
         * [build.sh](#buildsh)
      * [secrets](#secrets)
         * [deploy.sh](#deploysh)
         * [process.sh](#processsh)
      * [ssl](#ssl)
         * [upload_certificate.sh](#upload_certificatesh)
      * [ssm](#ssm)
         * [parameter_delete_path.sh](#parameter_delete_pathsh)
         * [parameter_delete.sh](#parameter_deletesh)
         * [parameter_get_file_multi_part.sh](#parameter_get_file_multi_partsh)
         * [parameter_get_file.sh](#parameter_get_filesh)
         * [parameter_get.sh](#parameter_getsh)
         * [parameter_put_file_multi_part.sh](#parameter_put_file_multi_partsh)
         * [parameter_put_file.sh](#parameter_put_filesh)
         * [parameter_put.sh](#parameter_putsh)
      * [utility](#utility)
         * [install.sh](#installsh)
         * [readme_generator.sh](#readme_generatorsh)
         * [replace_placeholders.sh](#replace_placeholderssh)
   * [Functions](#functions)
      * [aws/ami.inc](#awsamiinc)
         * [ami_copy()](#ami_copy)
         * [ami_create()](#ami_create)
         * [ami_description()](#ami_description)
         * [ami_id_exists()](#ami_id_exists)
         * [ami_name()](#ami_name)
         * [ami_name_exists()](#ami_name_exists)
         * [ami_state()](#ami_state)
         * [get_latest_ami_amazon_linux_hvm()](#get_latest_ami_amazon_linux_hvm)
         * [get_latest_ami_amazon_linux2_hvm()](#get_latest_ami_amazon_linux2_hvm)
         * [instance_id_exists()](#instance_id_exists)
      * [aws/cloudformation.inc](#awscloudformationinc)
         * [cloudformation_deploy()](#cloudformation_deploy)
         * [cloudformation_get_outputs()](#cloudformation_get_outputs)
         * [cloudformation_get_outputs_silent()](#cloudformation_get_outputs_silent)
         * [cloudformation_get_stack_attribute()](#cloudformation_get_stack_attribute)
         * [cloudformation_poll_status()](#cloudformation_poll_status)
         * [cloudformation_stack_exists()](#cloudformation_stack_exists)
         * [cloudformation_validate_template()](#cloudformation_validate_template)
         * [display_variables_details()](#display_variables_details)
         * [load_variables_details_from_file_keyvalue()](#load_variables_details_from_file_keyvalue)
         * [load_variables_details_from_file()](#load_variables_details_from_file)
         * [sam_package()](#sam_package)
      * [aws/cloudfront.inc](#awscloudfrontinc)
         * [cloudfront_invalidate_cache()](#cloudfront_invalidate_cache)
      * [aws/codedeploy.inc](#awscodedeployinc)
         * [codedeploy_create_bootstrap_deployment_group()](#codedeploy_create_bootstrap_deployment_group)
         * [codedeploy_create_deployment()](#codedeploy_create_deployment)
         * [codedeploy_delete_deployment_group()](#codedeploy_delete_deployment_group)
         * [codedeploy_get_current_revision_json()](#codedeploy_get_current_revision_json)
         * [codedeploy_get_deployed_revisions()](#codedeploy_get_deployed_revisions)
         * [codedeploy_get_deployment()](#codedeploy_get_deployment)
         * [codedeploy_get_deployment_revision_s3_bucket()](#codedeploy_get_deployment_revision_s3_bucket)
         * [codedeploy_get_deployment_revision_s3_key()](#codedeploy_get_deployment_revision_s3_key)
         * [codedeploy_get_service_role_arn()](#codedeploy_get_service_role_arn)
         * [codedeploy_register_revision()](#codedeploy_register_revision)
      * [aws/ec2.inc](#awsec2inc)
         * [ec2_get_availability_zone()](#ec2_get_availability_zone)
         * [ec2_get_ipaddress_private()](#ec2_get_ipaddress_private)
         * [ec2_harden_default_security_group()](#ec2_harden_default_security_group)
         * [ec2_import_keypair()](#ec2_import_keypair)
         * [ec2_instance_connect_send_ssh_key()](#ec2_instance_connect_send_ssh_key)
         * [ec2_source_destination_check_disable()](#ec2_source_destination_check_disable)
         * [ec2_tag_resource()](#ec2_tag_resource)
         * [ec2_tag_resource_from_file()](#ec2_tag_resource_from_file)
         * [ec2_vpc_route_update()](#ec2_vpc_route_update)
      * [aws/elasticache_memcached.inc](#awselasticache_memcachedinc)
         * [load_info_memcached()](#load_info_memcached)
         * [memcached_create()](#memcached_create)
         * [memcached_delete()](#memcached_delete)
         * [memcached_get_current_cache_cluster_id()](#memcached_get_current_cache_cluster_id)
         * [memcached_get_endpoint()](#memcached_get_endpoint)
         * [memcached_get_info()](#memcached_get_info)
         * [memcached_get_status()](#memcached_get_status)
         * [memcached_poll_status()](#memcached_poll_status)
         * [memcached_set_dns()](#memcached_set_dns)
      * [aws/elasticache_redis.inc](#awselasticache_redisinc)
         * [load_info_redis()](#load_info_redis)
         * [redis_create()](#redis_create)
         * [redis_delete()](#redis_delete)
         * [redis_get_current_replication_group_id()](#redis_get_current_replication_group_id)
         * [redis_get_endpoint()](#redis_get_endpoint)
         * [redis_get_info()](#redis_get_info)
         * [redis_get_latest_snapshot_name()](#redis_get_latest_snapshot_name)
         * [redis_get_replication_group_endpoint_read()](#redis_get_replication_group_endpoint_read)
         * [redis_get_replication_group_endpoint_write()](#redis_get_replication_group_endpoint_write)
         * [redis_get_status()](#redis_get_status)
         * [redis_poll_status()](#redis_poll_status)
         * [redis_set_dns()](#redis_set_dns)
      * [aws/eni.inc](#awseniinc)
         * [eni_attach()](#eni_attach)
         * [eni_detach()](#eni_detach)
         * [eni_return_attachment_id()](#eni_return_attachment_id)
         * [eni_return_status()](#eni_return_status)
      * [aws/iam.inc](#awsiaminc)
         * [attach_iam_policy_to_role()](#attach_iam_policy_to_role)
         * [create_iam_policy()](#create_iam_policy)
         * [create_iam_role()](#create_iam_role)
         * [delete_iam_policy()](#delete_iam_policy)
         * [delete_iam_role()](#delete_iam_role)
         * [detach_iam_policy_from_role()](#detach_iam_policy_from_role)
         * [return_iam_policy_arn()](#return_iam_policy_arn)
         * [return_iam_policy_attached_to_role()](#return_iam_policy_attached_to_role)
         * [return_iam_role_arn()](#return_iam_role_arn)
      * [aws/initialize.inc](#awsinitializeinc)
      * [aws/kms.inc](#awskmsinc)
         * [kms_decrypt_file()](#kms_decrypt_file)
         * [kms_decrypt_string()](#kms_decrypt_string)
         * [kms_encrypt_file()](#kms_encrypt_file)
         * [kms_encrypt_string()](#kms_encrypt_string)
         * [kms_generate_envelope_key()](#kms_generate_envelope_key)
      * [aws/lambda.inc](#awslambdainc)
         * [create_lambda_function()](#create_lambda_function)
         * [delete_lambda_function()](#delete_lambda_function)
         * [return_lambda_arn()](#return_lambda_arn)
      * [aws/loadbalancer.inc](#awsloadbalancerinc)
         * [elbv2_deregister_target()](#elbv2_deregister_target)
         * [elbv2_register_target()](#elbv2_register_target)
      * [aws/metadata.inc](#awsmetadatainc)
         * [aws_metadata_account_id()](#aws_metadata_account_id)
         * [aws_metadata_account_id_from_cli()](#aws_metadata_account_id_from_cli)
         * [aws_metadata_auto_scaling_group_name()](#aws_metadata_auto_scaling_group_name)
         * [aws_metadata_get_tag()](#aws_metadata_get_tag)
         * [aws_metadata_hostname()](#aws_metadata_hostname)
         * [aws_metadata_hostname_private()](#aws_metadata_hostname_private)
         * [aws_metadata_hostname_public()](#aws_metadata_hostname_public)
         * [aws_metadata_instance_id()](#aws_metadata_instance_id)
         * [aws_metadata_ipaddress_private()](#aws_metadata_ipaddress_private)
         * [aws_metadata_ipaddress_public()](#aws_metadata_ipaddress_public)
         * [aws_metadata_mac()](#aws_metadata_mac)
         * [aws_metadata_region()](#aws_metadata_region)
         * [aws_metadata_route_id_private()](#aws_metadata_route_id_private)
         * [aws_metadata_route_id_public()](#aws_metadata_route_id_public)
         * [aws_metadata_vpc_cidr()](#aws_metadata_vpc_cidr)
         * [aws_metadata_vpc_dns()](#aws_metadata_vpc_dns)
         * [aws_metadata_vpc_id()](#aws_metadata_vpc_id)
      * [aws/rds_aurora_postgresql.inc](#awsrds_aurora_postgresqlinc)
         * [load_info_aurora_postgresql()](#load_info_aurora_postgresql)
         * [aurora_postgresql_create_cluster()](#aurora_postgresql_create_cluster)
         * [aurora_postgresql_create_cluster_from_snapshot()](#aurora_postgresql_create_cluster_from_snapshot)
         * [aurora_postgresql_create_database()](#aurora_postgresql_create_database)
         * [aurora_postgresql_create_from_snapshot()](#aurora_postgresql_create_from_snapshot)
         * [aurora_postgresql_execute_sql()](#aurora_postgresql_execute_sql)
         * [aurora_postgresql_set_dns()](#aurora_postgresql_set_dns)
         * [aurora_postgresql_verify_connection()](#aurora_postgresql_verify_connection)
      * [aws/rds.inc](#awsrdsinc)
         * [rds_delete_cluster()](#rds_delete_cluster)
         * [rds_delete()](#rds_delete)
         * [rds_delete_call()](#rds_delete_call)
         * [rds_delete_cluster_member()](#rds_delete_cluster_member)
         * [rds_get_cluster_endpoint_read()](#rds_get_cluster_endpoint_read)
         * [rds_get_cluster_endpoint_write()](#rds_get_cluster_endpoint_write)
         * [rds_get_cluster_membership()](#rds_get_cluster_membership)
         * [rds_get_cluster_status()](#rds_get_cluster_status)
         * [rds_get_current_cluster_id()](#rds_get_current_cluster_id)
         * [rds_get_current_database_id()](#rds_get_current_database_id)
         * [rds_get_database_replica_ids()](#rds_get_database_replica_ids)
         * [rds_get_endpoint()](#rds_get_endpoint)
         * [rds_get_engine_version()](#rds_get_engine_version)
         * [rds_get_latest_cluster_snapshot_arn()](#rds_get_latest_cluster_snapshot_arn)
         * [rds_get_latest_snapshot_arn()](#rds_get_latest_snapshot_arn)
         * [rds_get_parameter_group()](#rds_get_parameter_group)
         * [rds_get_status()](#rds_get_status)
         * [rds_is_db_pending_reboot()](#rds_is_db_pending_reboot)
         * [rds_poll_cluster_status()](#rds_poll_cluster_status)
         * [rds_poll_status()](#rds_poll_status)
         * [rds_reboot()](#rds_reboot)
         * [rds_set_backup_policy()](#rds_set_backup_policy)
         * [rds_set_engine_version()](#rds_set_engine_version)
         * [rds_set_maintenance_policy()](#rds_set_maintenance_policy)
         * [rds_set_parameter_group()](#rds_set_parameter_group)
         * [rds_set_security_group()](#rds_set_security_group)
         * [rds_update_password()](#rds_update_password)
      * [aws/rds_mysql.inc](#awsrds_mysqlinc)
         * [load_info_mysql()](#load_info_mysql)
         * [mysql_create()](#mysql_create)
         * [mysql_create_from_snapshot()](#mysql_create_from_snapshot)
         * [mysql_create_replica()](#mysql_create_replica)
         * [mysql_execute_sql()](#mysql_execute_sql)
         * [mysql_execute_sql_output_to_file()](#mysql_execute_sql_output_to_file)
         * [mysql_set_dns()](#mysql_set_dns)
         * [mysql_verify_connection()](#mysql_verify_connection)
      * [aws/rds_postgresql.inc](#awsrds_postgresqlinc)
         * [load_info_postgresql()](#load_info_postgresql)
         * [postgresql_create()](#postgresql_create)
         * [postgresql_create_from_snapshot()](#postgresql_create_from_snapshot)
         * [postgresql_create_replica()](#postgresql_create_replica)
         * [postgresql_execute_sql()](#postgresql_execute_sql)
         * [postgresql_set_dns()](#postgresql_set_dns)
         * [postgresql_verify_connection()](#postgresql_verify_connection)
      * [aws/route53.inc](#awsroute53inc)
         * [route53_delete_record()](#route53_delete_record)
         * [route53_delete_record_call()](#route53_delete_record_call)
         * [route53_get_record()](#route53_get_record)
         * [route53_upsert_record()](#route53_upsert_record)
      * [aws/s3.inc](#awss3inc)
         * [s3_cp_download()](#s3_cp_download)
         * [s3_cp_upload()](#s3_cp_upload)
         * [s3_delete()](#s3_delete)
         * [s3_move_in_bucket()](#s3_move_in_bucket)
         * [s3_sync_download()](#s3_sync_download)
         * [s3_sync_upload()](#s3_sync_upload)
         * [s3_verify()](#s3_verify)
      * [aws/s3_website.inc](#awss3_websiteinc)
         * [load_info_s3_website()](#load_info_s3_website)
         * [process_s3_website()](#process_s3_website)
      * [aws/ssl.inc](#awssslinc)
         * [acm_certificate_exists()](#acm_certificate_exists)
         * [acm_import_certificate()](#acm_import_certificate)
         * [acm_tag_certificate()](#acm_tag_certificate)
         * [iam_certificate_exists()](#iam_certificate_exists)
         * [iam_import_certificate()](#iam_import_certificate)
         * [ssl_download_certificate()](#ssl_download_certificate)
      * [aws/ssm.inc](#awsssminc)
         * [parameter_delete()](#parameter_delete)
         * [parameter_exists()](#parameter_exists)
         * [parameter_get()](#parameter_get)
         * [parameter_get_file()](#parameter_get_file)
         * [parameter_get_file_multi_part()](#parameter_get_file_multi_part)
         * [parameter_get_file_silent()](#parameter_get_file_silent)
         * [parameter_get_silent()](#parameter_get_silent)
         * [parameter_path_delete()](#parameter_path_delete)
         * [parameter_put()](#parameter_put)
         * [parameter_put_file()](#parameter_put_file)
         * [parameter_put_file_multi_part()](#parameter_put_file_multi_part)
         * [parameter_tag()](#parameter_tag)
         * [parameter_tag_path()](#parameter_tag_path)
         * [parameters_to_key_value_file()](#parameters_to_key_value_file)
         * [parameters_to_key_value_file_multipart()](#parameters_to_key_value_file_multipart)
         * [parameters_to_properties_file()](#parameters_to_properties_file)
      * [aws/vpc.inc](#awsvpcinc)
         * [vpc_peering_connection_enable_dns()](#vpc_peering_connection_enable_dns)
      * [core/builder.inc](#corebuilderinc)
         * [display_variables_object_lists()](#display_variables_object_lists)
         * [display_variables_parameter()](#display_variables_parameter)
         * [display_variables_project()](#display_variables_project)
         * [generate_file_details()](#generate_file_details)
         * [generate_file_parameters()](#generate_file_parameters)
         * [generate_file_tags()](#generate_file_tags)
         * [generate_file_targets()](#generate_file_targets)
         * [generate_file_template()](#generate_file_template)
         * [load_object_files()](#load_object_files)
         * [load_objects()](#load_objects)
         * [load_objects_base()](#load_objects_base)
         * [load_paradigm()](#load_paradigm)
         * [load_parameters()](#load_parameters)
         * [load_project()](#load_project)
         * [load_project_sam()](#load_project_sam)
         * [load_secrets()](#load_secrets)
         * [load_targets()](#load_targets)
         * [load_templates()](#load_templates)
         * [manifest_get_checksum()](#manifest_get_checksum)
         * [manifest_put_checksum()](#manifest_put_checksum)
      * [core/color.inc](#corecolorinc)
         * [color_lookup()](#color_lookup)
         * [color_text()](#color_text)
         * [debug_color_text()](#debug_color_text)
      * [core/common.inc](#corecommoninc)
         * [add_element_to_array()](#add_element_to_array)
         * [base64_decode()](#base64_decode)
         * [base64_encode()](#base64_encode)
         * [base64_encode_and_compress()](#base64_encode_and_compress)
         * [call_sleep()](#call_sleep)
         * [call_sleep_random()](#call_sleep_random)
         * [does_array_contain_element()](#does_array_contain_element)
         * [echo_key_value()](#echo_key_value)
         * [explode_variables_to_file()](#explode_variables_to_file)
         * [file_update_owner()](#file_update_owner)
         * [file_update_permissions()](#file_update_permissions)
         * [filesize_bytes_to_human_readable()](#filesize_bytes_to_human_readable)
         * [function_exists()](#function_exists)
         * [generate_temp_directory()](#generate_temp_directory)
         * [generate_temp_file()](#generate_temp_file)
         * [generate_uuid()](#generate_uuid)
         * [html_escape()](#html_escape)
         * [is_empty()](#is_empty)
         * [is_int()](#is_int)
         * [is_server_up()](#is_server_up)
         * [json_safe()](#json_safe)
         * [load_array_key_values_from_file()](#load_array_key_values_from_file)
         * [load_array_key_values_from_yaml_file()](#load_array_key_values_from_yaml_file)
         * [load_array_properties_from_file()](#load_array_properties_from_file)
         * [load_key_value_from_file()](#load_key_value_from_file)
         * [load_property_from_file()](#load_property_from_file)
         * [option_enabled()](#option_enabled)
         * [return_file_last_modify_timestamp()](#return_file_last_modify_timestamp)
         * [return_file_md5sum()](#return_file_md5sum)
         * [return_file_mime()](#return_file_mime)
         * [return_file_sha1sum()](#return_file_sha1sum)
         * [return_file_sha256sum()](#return_file_sha256sum)
         * [return_filesize_of_file()](#return_filesize_of_file)
         * [return_filesize_of_file_in_bytes()](#return_filesize_of_file_in_bytes)
         * [return_header_string()](#return_header_string)
         * [return_parameter_string()](#return_parameter_string)
         * [return_yaml_string()](#return_yaml_string)
         * [source_include()](#source_include)
         * [sync_disks()](#sync_disks)
         * [time_elapsed()](#time_elapsed)
         * [timestamp_to_human_readable()](#timestamp_to_human_readable)
         * [to_lower()](#to_lower)
         * [to_upper()](#to_upper)
         * [trim()](#trim)
         * [uptime_greater_than()](#uptime_greater_than)
         * [url_decode()](#url_decode)
         * [url_encode()](#url_encode)
         * [verify_array_key_values()](#verify_array_key_values)
         * [version_to_integer()](#version_to_integer)
         * [write_key_value_to_file()](#write_key_value_to_file)
      * [core/logging.inc](#corelogginginc)
         * [line_break()](#line_break)
         * [log()](#log)
         * [log_add_from_file()](#log_add_from_file)
         * [log_error()](#log_error)
         * [log_file_content()](#log_file_content)
         * [log_highlight()](#log_highlight)
         * [log_load_color_child()](#log_load_color_child)
         * [log_load_color_parent()](#log_load_color_parent)
         * [log_notice()](#log_notice)
         * [log_quiet()](#log_quiet)
         * [log_success()](#log_success)
         * [log_warning()](#log_warning)
         * [verbose_display_date()](#verbose_display_date)
      * [core/script_logic.inc](#corescript_logicinc)
         * [cleanup()](#cleanup)
         * [exec_script()](#exec_script)
         * [exit_logic()](#exit_logic)
         * [exit_logic_skip_cleanup()](#exit_logic_skip_cleanup)
         * [exit_trap()](#exit_trap)
         * [exit_trap_ensure_graceful()](#exit_trap_ensure_graceful)
         * [lookup_exit_code()](#lookup_exit_code)
         * [return_script_purpose()](#return_script_purpose)
         * [return_script_version()](#return_script_version)
         * [sort_array_required_executeables()](#sort_array_required_executeables)
         * [sort_array_required_source_files()](#sort_array_required_source_files)
         * [start_logic()](#start_logic)
         * [usage()](#usage)
         * [usage_banner()](#usage_banner)
         * [usage_normalized_arguments()](#usage_normalized_arguments)
         * [verify_dependencies()](#verify_dependencies)
***

## Getting Started
These instructions describe how to get this project running on your machines for development and/or production use.

### Prerequisites
- Linux
- BASH shell

### Dependencies
- aws cli
- additional dependencies are required for specific Toolsets, please see the Required Executeables detail as indicated in the Library Function files

### Installing
Extract files
```
mkdir -p /opt/aws_builder
tar zxf aws_builder-latest.tgz -C /opt/aws_builder
```

Initialize Application
```
bash /opt/aws_builder/scripts/utility/install.sh -d /usr/local/sbin
```

## Scripts
Available Scripts
> The following arguments are available to all scripts

| Argument | Description |
| --- | --- |
| -h | Show help message
| -V | Show version number and exit
***
### ami
#### `amazon_linux2_encrypted.sh`
AMI - Amazon Linux 2 (encrypted)

| Argument | Description |
| --- | --- |
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ami/amazon_linux2_encrypted.sh <arguments>
alias:  aws_builder.ami.amazon_linux2_encrypted <arguments>
```
***
### cloudformation
#### `build.sh`
Builds CloudFormation template and dependencies from AWS Builder project

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | AWS Builder Project File
| -s &lt;s3_bucket_temp&gt; | S3 Bucket to Place Temporary files
| -d &lt;directory&gt; | Output Directory
| -B &lt;directory&gt; | Builder Directory Override (optional)
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/cloudformation/build.sh <arguments>
alias:  aws_builder.cloudformation.build <arguments>
```
#### `deploy.sh`
Deploys CloudFormation Template

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | CloudFormation Template File
| -s &lt;s3_bucket_temp&gt; | S3 Bucket to Place Temporary files
| -D &lt;dynamodb_table&gt; | CloudFormation Manifest DynamoDB Table (optional)
| -R &lt;dynamodb_region&gt; | CloudFormation Manifest DynamoDB Region (optional)
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/cloudformation/deploy.sh <arguments>
alias:  aws_builder.cloudformation.deploy <arguments>
```
#### `process.sh`
Processes directory of CloudFormation files

| Argument | Description |
| --- | --- |
| -d &lt;directory&gt; | CloudFormation Source Directory
| -s &lt;s3_bucket_temp&gt; | S3 Bucket to Place Temporary files
| -D &lt;dynamodb_table&gt; | CloudFormation Manifest DynamoDB Table (optional)
| -R &lt;dynamodb_region&gt; | CloudFormation Manifest DynamoDB Region (optional)
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/cloudformation/process.sh <arguments>
alias:  aws_builder.cloudformation.process <arguments>
```
#### `targets.sh`
Allows for additional processing which cannot occur during CloudFormation deployment

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | CloudFormation Targets File
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/cloudformation/targets.sh <arguments>
alias:  aws_builder.cloudformation.targets <arguments>
```
#### `validate.sh`
Validates of CloudFormation template

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | CloudFormation Template File
| -s &lt;s3_bucket_temp&gt; | S3 Bucket to Place Temporary files
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/cloudformation/validate.sh <arguments>
alias:  aws_builder.cloudformation.validate <arguments>
```
***
### cloudfront
#### `invalidate_cache.sh`
CloudFront (Invalidate Cache)

| Argument | Description |
| --- | --- |
| -s &lt;stack_name&gt; | Cloudformation Stack Name for CloudFront Deployment
| -p &lt;invalidation_path&gt; | Path to Invalidate
| -r &lt;aws_region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/cloudfront/invalidate_cache.sh <arguments>
alias:  aws_builder.cloudfront.invalidate_cache <arguments>
```
***
### codedeploy
#### `deploy_revision.sh`
CodeDeploy (Deploy Revision)

| Argument | Description |
| --- | --- |
| -c &lt;codedeploy_application&gt; | CodeDeploy Application Name
| -g &lt;codedeploy_deployment_group&gt; | CodeDeploy Deployment Group
| -d &lt;codedeploy_deployment_config&gt; | CodeDeploy Deployment Config
| -t &lt;codedeploy_timeout&gt; | CodeDeploy Timeout (in minutes, optional, defaults to 30 minutes)
| -f &lt;filename&gt; | Revision Filename
| -b &lt;bucket_name&gt; | S3 Bucket Name
| -D &lt;deployment_desc&gt; | Deployment Description (optional)
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/codedeploy/deploy_revision.sh <arguments>
alias:  aws_builder.codedeploy.deploy_revision <arguments>
```
#### `register_revision.sh`
CodeDeploy (Register Revision)

| Argument | Description |
| --- | --- |
| -c &lt;codedeploy_application&gt; | CodeDeploy Application Name
| -f &lt;file&gt; | Revision File
| -b &lt;bucket_name&gt; | S3 Bucket Name
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/codedeploy/register_revision.sh <arguments>
alias:  aws_builder.codedeploy.register_revision <arguments>
```
***
### ec2
#### `generate_ssh_keypair.sh`
EC2 (Generate SSH Keypair)

| Argument | Description |
| --- | --- |
| -p &lt;purpose_identifier&gt; | Purpose Identifier (ec2-user, app_name_abbr, etc)
| -o &lt;organization_abbr&gt; | Organization Abbreviation
| -d &lt;directory&gt; | Output Directory
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ec2/generate_ssh_keypair.sh <arguments>
alias:  aws_builder.ec2.generate_ssh_keypair <arguments>
```
***
### elasticache/redis
#### `create.sh`
Creates Redis Elasticache (supports snapshots)

| Argument | Description |
| --- | --- |
| -s &lt;stack_name&gt; | Cloudformation Stack Name
| -t &lt;timeout&gt; | Timeout (in minutes)
| -S &lt;snapshot_arn&gt; | Snapshot Name to create new Redis (optional)
| -c &lt;cluster_name&gt; | Cluster Name to use snapshots from to create new Redis (optional)
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/elasticache/redis/create.sh <arguments>
alias:  aws_builder.elasticache.redis.create <arguments>
```
***
### kms
#### `decrypt_file.sh`
KMS - Decrypt File

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | File to decrypt
| -d &lt;directory&gt; | Output Directory
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/kms/decrypt_file.sh <arguments>
alias:  aws_builder.kms.decrypt_file <arguments>
```
#### `encrypt_file.sh`
KMS - Encrypt File

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | File to encrypt
| -d &lt;directory&gt; | Output Directory
| -k &lt;kms_key_id&gt; | KMS Key ID
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/kms/encrypt_file.sh <arguments>
alias:  aws_builder.kms.encrypt_file <arguments>
```
***
### rds/aurorapostgresql
#### `create.sh`
Creates Aurora PostgreSQL Database (supports snapshots and SQL execution)

| Argument | Description |
| --- | --- |
| -s &lt;stack_name&gt; | Cloudformation Stack Name
| -t &lt;timeout&gt; | Timeout (in minutes)
| -S &lt;snapshot_arn&gt; | Snapshot ARN to create new Database (optional)
| -c &lt;cluster_id&gt; | Cluster ID to use snapshots from to create new Database (optional)
| -e &lt;sql_execution_directory&gt; | Directory to execute SQL scripts from (optional)
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/rds/aurorapostgresql/create.sh <arguments>
alias:  aws_builder.rds.aurorapostgresql.create <arguments>
```
***
### rds/mysql
#### `create.sh`
Creates MySQL Database (supports snapshots and SQL execution)

| Argument | Description |
| --- | --- |
| -s &lt;stack_name&gt; | Cloudformation Stack Name
| -t &lt;timeout&gt; | Timeout (in minutes)
| -S &lt;snapshot_arn&gt; | Snapshot ARN to create new Database (optional)
| -d &lt;database_id&gt; | Database ID to use snapshots from to create new Database (optional)
| -e &lt;sql_execution_directory&gt; | Directory to execute SQL scripts from (optional)
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/rds/mysql/create.sh <arguments>
alias:  aws_builder.rds.mysql.create <arguments>
```
***
### rds/postgresql
#### `create.sh`
Creates PostgreSQL Database (supports snapshots and SQL execution)

| Argument | Description |
| --- | --- |
| -s &lt;stack_name&gt; | Cloudformation Stack Name
| -t &lt;timeout&gt; | Timeout (in minutes)
| -S &lt;snapshot_arn&gt; | Snapshot ARN to create new Database (optional)
| -d &lt;database_id&gt; | Database ID to use snapshots from to create new Database (optional)
| -e &lt;sql_execution_directory&gt; | Directory to execute SQL scripts from (optional)
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/rds/postgresql/create.sh <arguments>
alias:  aws_builder.rds.postgresql.create <arguments>
```
***
### rds/tools
#### `download_logs.sh`
RDS - Retrieves RDS Logs for specified Database

| Argument | Description |
| --- | --- |
| -d &lt;database_id&gt; | Database ID
| -D &lt;directory&gt; | Directory to store logs
| -t &lt;time_in_hours&gt; | Pull logs for X hours (optional, defaults to all logs)
| -z | Compress Logs
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/rds/tools/download_logs.sh <arguments>
alias:  aws_builder.rds.tools.download_logs <arguments>
```
#### `sync_all_logs_to_s3.sh`
RDS - Syncs all Logs from all Databases to S3

| Argument | Description |
| --- | --- |
| -s &lt;s3_bucket&gt; | S3 Bucket
| -p &lt;s3_prefix&gt; | S3 Prefix
| -t &lt;time_in_hours&gt; | Pull logs for X hours (optional, defaults to all logs)
| -r &lt;region&gt; | AWS Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/rds/tools/sync_all_logs_to_s3.sh <arguments>
alias:  aws_builder.rds.tools.sync_all_logs_to_s3 <arguments>
```
***
### s3
#### `create_temp_bucket.sh`
S3 (Create Temp Bucket)

| Argument | Description |
| --- | --- |
| -b &lt;bucket_name&gt; | S3 Bucket Name (recommended to include Account ID in name. example
| -o &lt;owner&gt; | Owner string for Tagging
| -c &lt;contact_email&gt; | Contact Email for Tagging
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/s3/create_temp_bucket.sh <arguments>
alias:  aws_builder.s3.create_temp_bucket <arguments>
```
#### `download_file.sh`
S3 (Download File)

| Argument | Description |
| --- | --- |
| -b &lt;bucket_name&gt; | S3 Bucket Name
| -f &lt;source_file&gt; | File in S3 Bucket
| -F &lt;local_file&gt; | Local File
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/s3/download_file.sh <arguments>
alias:  aws_builder.s3.download_file <arguments>
```
#### `move_file.sh`
S3 (Move File)

| Argument | Description |
| --- | --- |
| -b &lt;bucket_name&gt; | S3 Bucket Name
| -f &lt;source_file&gt; | File in S3 Bucket (Source)
| -F &lt;target_file&gt; | File in S3 Bucket (Target)
| -s | Enable server side encryption
| -r | Enable Reduced Redundancy
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/s3/move_file.sh <arguments>
alias:  aws_builder.s3.move_file <arguments>
```
#### `upload_file.sh`
S3 (Upload File)

| Argument | Description |
| --- | --- |
| -b &lt;bucket_name&gt; | S3 Bucket Name
| -f &lt;destination_file&gt; | File in S3 Bucket
| -F &lt;local_file&gt; | Local File
| -s | Enable server side encryption
| -r | Enable Reduced Redundancy
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/s3/upload_file.sh <arguments>
alias:  aws_builder.s3.upload_file <arguments>
```
#### `upload_website.sh`
S3 (Upload Website)

| Argument | Description |
| --- | --- |
| -r &lt;aws_region&gt; | AWS Region
| -s &lt;stack_name&gt; | Cloudformation Stack Name
| -f &lt;file&gt; | s3_website.yml Template
| -d &lt;source_directory&gt; | Static site files directory
| -F | Force Sync (optional, defaults to no)
| -p &lt;file&gt; | Placeholder Var file (optional, if specified, will replace placeholders in files located in source directory)
| -i &lt;invalidation_path&gt; | Invalidation Path (optional, for use when specifying S3 Prefix and forcing an invalidation)
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/s3/upload_website.sh <arguments>
alias:  aws_builder.s3.upload_website <arguments>
```
***
### sam
#### `build.sh`
Builds SAM template and dependencies from AWS Builder SAM project

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | AWS Builder SAM Project File
| -s &lt;s3_bucket_temp&gt; | S3 Bucket to Place Temporary files
| -a &lt;s3_bucket_artifacts&gt; | S3 Bucket to Artifacts
| -d &lt;directory&gt; | Output Directory
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/sam/build.sh <arguments>
alias:  aws_builder.sam.build <arguments>
```
***
### secrets
#### `deploy.sh`
Deploys secrets file and uploads data to AWS as Encrypted SSM Parameters

| Argument | Description |
| --- | --- |
| -f &lt;file&gt; | Secrets File
| -D &lt;dynamodb_table&gt; | Secrets Manifest DynamoDB Table (optional)
| -R &lt;dynamodb_region&gt; | Secrets Manifest DynamoDB Region (optional)
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/secrets/deploy.sh <arguments>
alias:  aws_builder.secrets.deploy <arguments>
```
#### `process.sh`
Batch processes directory of secrets files

| Argument | Description |
| --- | --- |
| -d &lt;directory&gt; | Secrets Source Directory
| -D &lt;dynamodb_table&gt; | Secrets Manifest DynamoDB Table (optional)
| -R &lt;dynamodb_region&gt; | Secrets Manifest DynamoDB Region (optional)
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/secrets/process.sh <arguments>
alias:  aws_builder.secrets.process <arguments>
```
***
### ssl
#### `upload_certificate.sh`
SSL - Upload Certificate

| Argument | Description |
| --- | --- |
| -n &lt;name&gt; | SSL Certificate Name
| -d &lt;directory&gt; | SSL Certificate Directory
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssl/upload_certificate.sh <arguments>
alias:  aws_builder.ssl.upload_certificate <arguments>
```
***
### ssm
#### `parameter_delete_path.sh`
Parameter (Delete Path)

| Argument | Description |
| --- | --- |
| -p &lt;parameter_path&gt; | Parameter Path
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_delete_path.sh <arguments>
alias:  aws_builder.ssm.parameter_delete_path <arguments>
```
#### `parameter_delete.sh`
Parameter (Delete)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_delete.sh <arguments>
alias:  aws_builder.ssm.parameter_delete <arguments>
```
#### `parameter_get_file_multi_part.sh`
Parameter (Get File Multi-Part)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -f &lt;filename&gt; | Output file
| -r &lt;region&gt; | Region
| -d | Disable decryption
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_get_file_multi_part.sh <arguments>
alias:  aws_builder.ssm.parameter_get_file_multi_part <arguments>
```
#### `parameter_get_file.sh`
Parameter (Get File)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -f &lt;filename&gt; | Output file
| -r &lt;region&gt; | Region
| -d | Disable decryption
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_get_file.sh <arguments>
alias:  aws_builder.ssm.parameter_get_file <arguments>
```
#### `parameter_get.sh`
Parameter (Get)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -r &lt;region&gt; | Region
| -d | Disable decryption
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_get.sh <arguments>
alias:  aws_builder.ssm.parameter_get <arguments>
```
#### `parameter_put_file_multi_part.sh`
Parameter (Put File)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -f &lt;filename&gt; | File Target
| -k &lt;key&gt; | Encryption key to use &lt;optional, defaults to AWS Account Default key&gt;
| -d &lt;description&gt; | Description &lt;optional&gt;
| -D | Disable Parameter replacement
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_put_file_multi_part.sh <arguments>
alias:  aws_builder.ssm.parameter_put_file_multi_part <arguments>
```
#### `parameter_put_file.sh`
Parameter (Put File)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -f &lt;filename&gt; | File Target
| -k &lt;key&gt; | Encryption key to use &lt;optional, defaults to AWS Account Default key&gt;
| -d &lt;description&gt; | Description &lt;optional&gt;
| -D | Disable Parameter replacement
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_put_file.sh <arguments>
alias:  aws_builder.ssm.parameter_put_file <arguments>
```
#### `parameter_put.sh`
Parameter (Put)

| Argument | Description |
| --- | --- |
| -p &lt;parameter&gt; | Parameter
| -s &lt;string&gt; | String to attach to parameter
| -k &lt;key&gt; | Encryption key to use &lt;optional, defaults to AWS Account Default key&gt;
| -d &lt;description&gt; | Description &lt;optional&gt;
| -D | Disable Parameter replacement
| -r &lt;region&gt; | Region
| -P &lt;profile&gt; | AWS Profile (optional)
```
script: /opt/aws_builder/scripts/ssm/parameter_put.sh <arguments>
alias:  aws_builder.ssm.parameter_put <arguments>
```
***
### utility
#### `install.sh`
Installs AWS Builder Functionality

| Argument | Description |
| --- | --- |
| -d &lt;alias_links_directory&gt; | Alias Links Directory (optional, if not specified, no aliases will be generated
```
script: /opt/aws_builder/scripts/utility/install.sh <arguments>
alias:  aws_builder.utility.install <arguments>
```
#### `readme_generator.sh`
Scans AWS Builder application directory and generates an appropriate README.md

| Argument | Description |
| --- | --- |
| -f &lt;filename&gt; | Output file (example
```
script: /opt/aws_builder/scripts/utility/readme_generator.sh <arguments>
alias:  aws_builder.utility.readme_generator <arguments>
```
#### `replace_placeholders.sh`
Replace Placeholder Variables in all files in specified directory

| Argument | Description |
| --- | --- |
| -d &lt;directory&gt; | Directory to Scan
| -p &lt;file&gt; | Placeholder Var file
| -i &lt;file&gt; | Placeholder Info file
```
script: /opt/aws_builder/scripts/utility/replace_placeholders.sh <arguments>
alias:  aws_builder.utility.replace_placeholders <arguments>
```

## Functions
Available Functions
***
### aws/ami.inc
Collection of functions related to AMI

#### `ami_copy()`
- Creates AMI based on Image ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass newly created AMI ID to
| $2 | Source AMI ID
| $3 | AMI Name
| $4 | AMI Description
| $5 | Source Region
| $6 | Destination Region
| $7 | Encrypt AMI (default | yes)

#### `ami_create()`
- Creates AMI based on Image ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass newly created AMI ID to
| $2 | Image ID
| $3 | AMI Name Prepend String
| $4 | AMI Description Prepend String
| $5 | Region

#### `ami_description()`
- Obtains description for AMI

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | AMI ID
| $3 | Region

#### `ami_id_exists()`
- Returns true if ami_id exists

| Arg | Description |
| --- | --- |
| $1 | AMI ID
| $2 | Region

#### `ami_name()`
- Obtains name for AMI

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | AMI ID
| $3 | Region

#### `ami_name_exists()`
- Returns true if ami_name exists

| Arg | Description |
| --- | --- |
| $1 | AMI Name
| $2 | Region

#### `ami_state()`
- Obtains state for AMI

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | AMI ID
| $3 | Region

#### `get_latest_ami_amazon_linux_hvm()`
- Retrieves latest AMI ID [Amazon Linux (HVM)]

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass AMI ID to
| $2 | Region

#### `get_latest_ami_amazon_linux2_hvm()`
- Retrieves latest AMI ID [Amazon Linux 2 (HVM)]

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass AMI ID to
| $2 | Region

#### `instance_id_exists()`
- Returns true if instance_id exists

| Arg | Description |
| --- | --- |
| $1 | Instance ID
| $2 | Region
***
### aws/cloudformation.inc
Collection of functions related to AWS (CloudFormation)

#### `cloudformation_deploy()`
- Deploys CloudFormation Template to AWS

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass StackId to
| $2 | Stack Name
| $3 | Template file
| $4 | Parameters file
| $5 | Tags file
| $6 | S3 Bucket (Temp)
| $7 | Region

#### `cloudformation_get_outputs()`
- Retrieves outputs from CloudFormation Stack and dumps them into a file in KEY=VALUE format

| Arg | Description |
| --- | --- |
| $1 | File to dump data to
| $2 | CloudFormation Stack Name
| $3 | Region

#### `cloudformation_get_outputs_silent()`
- Retrieves outputs from CloudFormation Stack and dumps them into a file in KEY=VALUE format
- Suppresses Log output, unless there is an error

| Arg | Description |
| --- | --- |
| $1 | File to dump data to
| $2 | CloudFormation Stack Name
| $3 | Region

#### `cloudformation_get_stack_attribute()`
- Retrieves Stack Attribute for specified CloudFormation stack

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Stack Name
| $3 | Attribute
| $4 | Fail with error on STACK_DOES_NOT_EXIST (yes/no)
| $5 | Capitalize Attribute Value (yes/no)
| $6 | Region

#### `cloudformation_poll_status()`
- Polls CloudFormation Stack and waits for a stable state

| Arg | Description |
| --- | --- |
| $1 | Stack Name
| $2 | Fail with error on STACK_DOES_NOT_EXIST (yes/no)
| $3 | Verification Timeout &lt;in minutes, defaults to 45 minutes&gt;
| $4 | Verification Time between polling &lt;in seconds, defaults to 30 seconds)
| $5 | Region

#### `cloudformation_stack_exists()`
- Checks to see if CloudFormation Stack exists

| Arg | Description |
| --- | --- |
| $1 | Stack Name
| $2 | Region

#### `cloudformation_validate_template()`
- Validates specified CloudFormation Template using the AWS API call 'validate-template'

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Template file
| $2 | S3 Bucket (temp)
| $3 | Region

#### `display_variables_details()`
- Displays Variables | Details

#### `load_variables_details_from_file_keyvalue()`
- Loads Variables | Details from KeyValue file

| Arg | Description |
| --- | --- |
| $1 | Input file

#### `load_variables_details_from_file()`
- Loads Variables | Details from Project file

| Arg | Description |
| --- | --- |
| $1 | Input file

#### `sam_package()`
- Packages SAM Template

| Arg | Description |
| --- | --- |
| $1 | Input Template file
| $2 | Output Template file
| $3 | S3 Bucket (Artifacts)
| $4 | S3 Prefix
| $5 | Region
***
### aws/cloudfront.inc
Collection of functions related to CloudFront

#### `cloudfront_invalidate_cache()`
- Invalidates CloudFront Distribution based on Path

| Arg | Description |
| --- | --- |
| $1 | CloudFront Distribution ID
| $2 | Path to Invalidate
| $3 | Region
***
### aws/codedeploy.inc
Collection of functions related to AWS (CodeDeploy)

#### `codedeploy_create_bootstrap_deployment_group()`
- Creates Bootstrap Deployment Group

| Arg | Description |
| --- | --- |
| $1 | CodeDeploy Application Name to base Bootstrap Deployment on
| $2 | CodeDeploy Deployment Group Name to base Bootstrap Deployment on
| $3 | Bootstrap Deployment UUID
| $4 | Region

#### `codedeploy_create_deployment()`
- Deploys specified Revision via CodeDeploy

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | CodeDeploy Application Name
| $3 | CodeDeploy Group Name
| $4 | CodeDeploy Deployment Config
| $5 | S3 Bucket
| $6 | Revision Location
| $7 | Deployment Description (optional)
| $8 | Region

#### `codedeploy_delete_deployment_group()`
- Deletes specified CodeDeploy Deployment Group

| Arg | Description |
| --- | --- |
| $1 | CodeDeploy Application Name
| $2 | CodeDeploy Group Name
| $3 | Region

#### `codedeploy_get_current_revision_json()`
- Retrieves current revision information from specified Deployment Group and dumps JSON data to specified file

| Arg | Description |
| --- | --- |
| $1 | File to pass Revision JSON data to
| $2 | CodeDeploy Application Name
| $3 | CodeDeploy Deployment Group
| $4 | Region

#### `codedeploy_get_deployed_revisions()`
- Retrieves deployed revisions and places the information in the specified file

| Arg | Description |
| --- | --- |
| $1 | CodeDeploy Application Name
| $2 | File to pass revisions to
| $3 | Region

#### `codedeploy_get_deployment()`
- Tracks specified Deployment Revision via CodeDeploy

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Deployment ID
| $3 | Region

#### `codedeploy_get_deployment_revision_s3_bucket()`
- Returns S3 Bucket for specified Deployment Group Revision

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | CodeDeploy Application Name
| $3 | CodeDeploy Group Name
| $4 | Region

#### `codedeploy_get_deployment_revision_s3_key()`
- Returns S3 Key for specified Deployment Group Revision

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | CodeDeploy Application Name
| $3 | CodeDeploy Group Name
| $4 | Region

#### `codedeploy_get_service_role_arn()`
- Retrieves CodeDeploy Service Role ARN for specified CodeDeploy Deployment Group

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | CodeDeploy Application Name
| $3 | CodeDeploy Deployment Group
| $4 | Region

#### `codedeploy_register_revision()`
- Register Application Revision with CodeDeploy

| Arg | Description |
| --- | --- |
| $1 | CodeDeploy Application Name
| $2 | CodeDeploy S3 Bucket
| $3 | CodeDeploy S3 Key
| $4 | Region
***
### aws/ec2.inc
Collection of functions related to EC2

#### `ec2_get_availability_zone()`
- Returns Availability Zone of specified Instance

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Instance ID
| $3 | Region

#### `ec2_get_ipaddress_private()`
- Returns Private IP Address of specified Instance

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Instance ID
| $3 | Region

#### `ec2_harden_default_security_group()`
- Hardens Default Security Group by removing all rules, ensure all traffic is denied

| Arg | Description |
| --- | --- |
| $1 | Security Group ID
| $2 | Region

#### `ec2_import_keypair()`
- Uploads SSH Public Key for use with EC2 Instances

| Arg | Description |
| --- | --- |
| $1 | Instance ID
| $2 | Region

#### `ec2_instance_connect_send_ssh_key()`
- EC2 Instance Connect - Sends SSH Key for specified user to specified EC2 Instance

| Arg | Description |
| --- | --- |
| $1 | Instance ID
| $2 | OS User
| $3 | Region
| $4 | Availability Zone
| $5 | SSH Public Key

#### `ec2_source_destination_check_disable()`
- Disables Source Destination Check on specified instance

| Arg | Description |
| --- | --- |
| $1 | Instance ID
| $2 | Region

#### `ec2_tag_resource()`
- Tags Resource (EC2) with specified Key and Value

| Arg | Description |
| --- | --- |
| $1 | Resource ID
| $2 | Key
| $3 | Value
| $4 | Region

#### `ec2_tag_resource_from_file()`
- Tags Resource (EC2) with values from specified file

| Arg | Description |
| --- | --- |
| $1 | Resource ID
| $2 | Tags File
| $3 | Region

#### `ec2_vpc_route_update()`
- Updates VPC Route Table to route through specified Instance

| Arg | Description |
| --- | --- |
| $1 | Route Table ID
| $2 | Instance ID
| $3 | Region
***
### aws/elasticache_memcached.inc
Collection of functions related to AWS (ElastiCache - Memcached)

#### `load_info_memcached()`
- Loads Memcached Information into memory from CloudFormation Outputs file

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Outputs File

#### `memcached_create()`
- Creates Memcached

| Arg | Description |
| --- | --- |
| $1 | Region

#### `memcached_delete()`
- Delete Memcached

| Arg | Description |
| --- | --- |
| $1 | Memcached Reference ID
| $2 | Region

#### `memcached_get_current_cache_cluster_id()`
- Retrieves current Cache Cluster ID for Memcached

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Reference Name
| $3 | Region

#### `memcached_get_endpoint()`
- Retrieves Endpoint for specified Memcached

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Memcached Reference ID
| $3 | Region

#### `memcached_get_info()`
- Retrieves Memcached Info for specified endpoint using memcached-cli

| Arg | Description |
| --- | --- |
| $1 | Endpoint Address
| $2 | Port (optional, defaults to 11211)

#### `memcached_get_status()`
- Retrieves status for specified Memcached

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Memcached Reference ID
| $3 | Region

#### `memcached_poll_status()`
- Polls Memcached Status and waits for a stable state

| Arg | Description |
| --- | --- |
| $1 | Memcached Reference ID
| $2 | Fail with error on NOT_FOUND &lt;yes/no&gt; (defaults to yes)
| $3 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $4 | Verification Time between polling &lt;in seconds, defaults to 30 seconds)
| $5 | Region

#### `memcached_set_dns()`
- Sets DNS record for Memcached

| Arg | Description |
| --- | --- |
| $1 | Region
***
### aws/elasticache_redis.inc
Collection of functions related to AWS (ElastiCache - Redis)

#### `load_info_redis()`
- Loads Redis Information into memory from CloudFormation Outputs file

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Stack Name
| $2 | Region

#### `redis_create()`
- Creates Redis

| Arg | Description |
| --- | --- |
| $1 | Snapshot Name (optional, if not supplied, an empty Redis will be created)
| $2 | Region

#### `redis_delete()`
- Delete Redis

| Arg | Description |
| --- | --- |
| $1 | Redis Reference ID
| $2 | Region

#### `redis_get_current_replication_group_id()`
- Retrieves current Replication Group ID for Redis

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Reference Name
| $3 | Region

#### `redis_get_endpoint()`
- Retrieves Endpoint for specified Redis

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Redis Reference ID
| $3 | Region

#### `redis_get_info()`
- Retrieves Redis Info for specified endpoint using redis-cli

| Arg | Description |
| --- | --- |
| $1 | Endpoint Address
| $2 | Port (optional, defaults to 6379)

#### `redis_get_latest_snapshot_name()`
- Retrieves latest snapshot name for specified Redis Cluster

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Redis Reference ID
| $3 | Region

#### `redis_get_replication_group_endpoint_read()`
- Retrieves Redis Endpoint (Read) for specified Replication Group

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Replication Group ID
| $3 | Region

#### `redis_get_replication_group_endpoint_write()`
- Retrieves Redis Endpoint (Write) for specified Replication Group

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Replication Group ID
| $3 | Region

#### `redis_get_status()`
- Retrieves status for specified Redis

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Redis Reference ID
| $3 | Region

#### `redis_poll_status()`
- Polls Redis Status and waits for a stable state

| Arg | Description |
| --- | --- |
| $1 | Redis Reference ID
| $2 | Fail with error on NOT_FOUND &lt;yes/no&gt; (defaults to yes)
| $3 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $4 | Verification Time between polling &lt;in seconds, defaults to 30 seconds)
| $5 | Region

#### `redis_set_dns()`
- Sets DNS records (Read & Write) for Redis

| Arg | Description |
| --- | --- |
| $1 | Region
***
### aws/eni.inc
Collection of functions related to ENI

#### `eni_attach()`
- Attach ENI

| Arg | Description |
| --- | --- |
| $1 | ENI Name
| $2 | Instance ID
| $3 | Device ID (optional, defaults to 3)
| $4 | Region

#### `eni_detach()`
- Detach ENI

| Arg | Description |
| --- | --- |
| $1 | ENI Attachment ID
| $2 | Region

#### `eni_return_attachment_id()`
- Returns Attachment ID associated with ENI

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | ENI Name
| $3 | Region

#### `eni_return_status()`
- Returns Status associated with ENI

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | ENI Name
| $3 | Region
***
### aws/iam.inc
Collection of functions related to IAM

#### `attach_iam_policy_to_role()`
- Create Lambda Function

| Arg | Description |
| --- | --- |
| $1 | Policy Name
| $2 | Role Name
| $3 | Region

#### `create_iam_policy()`
- Create IAM Policy

| Arg | Description |
| --- | --- |
| $1 | Policy Name
| $2 | Policy Document File
| $3 | Region

#### `create_iam_role()`
- Create IAM Role

| Arg | Description |
| --- | --- |
| $1 | Role Name
| $2 | Trust Policy Document File
| $3 | Region

#### `delete_iam_policy()`
- Delete IAM Policy

| Arg | Description |
| --- | --- |
| $1 | Policy ARN
| $2 | Region

#### `delete_iam_role()`
- Delete IAM Role

| Arg | Description |
| --- | --- |
| $1 | Role Name
| $2 | Region

#### `detach_iam_policy_from_role()`
- Detaches IAM Policy from Role

| Arg | Description |
| --- | --- |
| $1 | Policy ARN
| $2 | Role Name
| $3 | Region

#### `return_iam_policy_arn()`
- Returns IAM Policy ARN

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Policy Name
| $3 | Region

#### `return_iam_policy_attached_to_role()`
- Returns attached IAM Policy attached to Role

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Role Name
| $3 | Region

#### `return_iam_role_arn()`
- Returns IAM Role ARN

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Role Name
| $3 | Region
***
### aws/initialize.inc
Initializes AWS CLI use for Scripts
***
### aws/kms.inc
Collection of functions related to AWS (KMS)

#### `kms_decrypt_file()`
- Decrypts file using KMS

| Arg | Description |
| --- | --- |
| $1 | File to Decrypt
| $2 | Output Directory
| $3 | Region

#### `kms_decrypt_string()`
- Decrypts string using KMS

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Encrypted String
| $3 | Mask Decrypted String &lt;yes/no, defaults to no&gt;
| $4 | Region

#### `kms_encrypt_file()`
- Encrypts file using KMS
- Generates encrypted file in specified directory using the Target Filename, appending '.enc' to the end of the filename

| Arg | Description |
| --- | --- |
| $1 | File to Encrypt
| $2 | Output Directory
| $3 | KMS Key to use
| $4 | Region

#### `kms_encrypt_string()`
- Encrypts string using KMS

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | String to encrypt
| $3 | KMS Key to use
| $4 | Region

#### `kms_generate_envelope_key()`
- Generates Envelope Key using specified KMS Key

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | KMS Key ID
| $3 | Region
***
### aws/lambda.inc
Collection of functions related to Lambda

#### `create_lambda_function()`
- Creates Lambda Function

| Arg | Description |
| --- | --- |
| $1 | Lambda Function Name
| $2 | Lambda json file
| $3 | Region

#### `delete_lambda_function()`
- Deletes Lambda Function

| Arg | Description |
| --- | --- |
| $1 | Lambda Function Name
| $2 | Region

#### `return_lambda_arn()`
- Returns Lambda Function ARN

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Lambda Function Name
| $3 | Region
***
### aws/loadbalancer.inc
Collection of functions related to AWS (LoadBalancer)

#### `elbv2_deregister_target()`
- Deregisters Instance from LoadBalancer Target Group (ELB v2)

| Arg | Description |
| --- | --- |
| $1 | Target Group ARN
| $2 | Instance ID
| $3 | Region

#### `elbv2_register_target()`
- Registers Instance to LoadBalancer Target Group (ELB v2)

| Arg | Description |
| --- | --- |
| $1 | Target Group ARN
| $2 | Instance ID
| $3 | Region
***
### aws/metadata.inc
Collection of functions related to AWS (Metadata)

#### `aws_metadata_account_id()`
- Returns AWS Account ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_account_id_from_cli()`
- Returns AWS Account ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_auto_scaling_group_name()`
- Returns AWS VPC ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Region
| $3 | Instance ID

#### `aws_metadata_get_tag()`
- Returns Instance Tag

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Key
| $3 | Region (optional, defaults to self region)
| $4 | Instance ID (optional, defaults to self instance ID)

#### `aws_metadata_hostname()`
- Returns Hostname FQDN (attempts to pull public hostname first, the falls back to internal)

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_hostname_private()`
- Returns local hostname fqdn

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_hostname_public()`
- Returns public hostname fqdn

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_instance_id()`
- Returns Instance ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_ipaddress_private()`
- Returns Private IP Address

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_ipaddress_public()`
- Returns Public IP Address

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_mac()`
- Returns MAC

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_region()`
- Returns AWS Region

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_route_id_private()`
- Returns Private Route ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Region
| $3 | Instance ID

#### `aws_metadata_route_id_public()`
- Returns Public Route ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Region
| $3 | Instance ID

#### `aws_metadata_vpc_cidr()`
- Returns AWS VPC CIDR Range

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_vpc_dns()`
- Returns AWS VPC DNS IP

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to

#### `aws_metadata_vpc_id()`
- Returns AWS VPC ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Region
| $3 | Instance ID
***
### aws/rds_aurora_postgresql.inc
Collection of functions related to AWS (RDS - Aurora PostgreSQL)

#### `load_info_aurora_postgresql()`
- Loads Aurora PostgreSQL Information into memory

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Stack Name
| $2 | Region

#### `aurora_postgresql_create_cluster()`
- Creates Aurora PostgreSQL Cluster

| Arg | Description |
| --- | --- |
| $1 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $2 | Region

#### `aurora_postgresql_create_cluster_from_snapshot()`
- Creates Aurora PostgreSQL Cluster from Snapshot

| Arg | Description |
| --- | --- |
| $1 | Snapshot ARN
| $2 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $3 | Region

#### `aurora_postgresql_create_database()`
- Creates Aurora PostgreSQL Database for Cluster
- Does not perform polling

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Region

#### `aurora_postgresql_create_from_snapshot()`
- Creates Aurora PostgreSQL Database from specified Snapshot

| Arg | Description |
| --- | --- |
| $1 | Snapshot ARN
| $2 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $3 | Region

#### `aurora_postgresql_execute_sql()`
- Attempts to connect to specified Aurora PostgreSQL Database in order to execute SQL file

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Name (optional, defaults to 'postgres')
| $5 | Database Port (optional, if not specified, defaults to DATABASE_DEFAULT_PORT)
| $6 | SQL File

#### `aurora_postgresql_set_dns()`
- Sets DNS records (Read & Write) for Aurora PostgreSQL Database

| Arg | Description |
| --- | --- |
| $1 | Region

#### `aurora_postgresql_verify_connection()`
- Attempts to connect to specified Aurora PostgreSQL Database in order to verify if database is available and credentials are correct

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Name (optional, defaults to 'postgres')
| $5 | Database Port (optional, if not specified, defaults to DATABASE_DEFAULT_PORT)
***
### aws/rds.inc
Collection of functions related to AWS (RDS)

#### `rds_delete_cluster()`
- Delete RDS Cluster

| Arg | Description |
| --- | --- |
| $1 | Cluster ID
| $2 | Take Snapshot (yes/no, defaults to no)
| $3 | Region

#### `rds_delete()`
- Delete RDS Database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Take Snapshot (yes/no, defaults to no)
| $3 | Region

#### `rds_delete_call()`
- Delete RDS Database (the actual CLI call to perform delete)

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Take Snapshot (yes/no, defaults to no)
| $3 | Region

#### `rds_delete_cluster_member()`
- Delete Cluster Member database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Region

#### `rds_get_cluster_endpoint_read()`
- Retrieves Cluster Endpoint (Read) for specified Cluster

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Cluster ID
| $3 | Region

#### `rds_get_cluster_endpoint_write()`
- Retrieves Cluster Endpoint (Write) for specified Cluster

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Cluster ID
| $3 | Region

#### `rds_get_cluster_membership()`
- Retrieves current list of Databases in specified Cluster

| Arg | Description |
| --- | --- |
| $1 | Array name to pass info to
| $2 | Cluster ID
| $3 | Region

#### `rds_get_cluster_status()`
- Retrieves status for specified Cluster

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Cluster ID
| $3 | Region

#### `rds_get_current_cluster_id()`
- Retrieves current Cluster ID for RDS

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Cluster Reference Name
| $3 | Region

#### `rds_get_current_database_id()`
- Retrieves current Database ID for RDS

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database Reference Name
| $3 | Region

#### `rds_get_database_replica_ids()`
- Retrieves current Database Replica IDs for RDS Database

| Arg | Description |
| --- | --- |
| $1 | Array name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_get_endpoint()`
- Retrieves Endpoint for specified Database

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_get_engine_version()`
- Retrieves engine version for specified Database

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_get_latest_cluster_snapshot_arn()`
- Retrieves latest snapshot ARN for specified Cluster ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_get_latest_snapshot_arn()`
- Retrieves latest snapshot ARN for specified Database ID

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_get_parameter_group()`
- Retrieves parameter group for specified Database

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_get_status()`
- Retrieves status for specified Database

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Database ID
| $3 | Region

#### `rds_is_db_pending_reboot()`
- Checks if Database is currently pending reboot

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Region

#### `rds_poll_cluster_status()`
- Polls RDS Cluster Status and waits for a stable state

| Arg | Description |
| --- | --- |
| $1 | Cluster ID
| $2 | Fail with error on NOT_FOUND &lt;yes/no&gt; (defaults to yes)
| $3 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $4 | Verification Time between polling &lt;in seconds, defaults to 30 seconds)
| $5 | Region

#### `rds_poll_status()`
- Polls RDS Database Status and waits for a stable state

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Fail with error on NOT_FOUND &lt;yes/no&gt; (defaults to yes)
| $3 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $4 | Verification Time between polling &lt;in seconds, defaults to 30 seconds)
| $5 | Region

#### `rds_reboot()`
- Reboots specified RDS Instance

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Enable Failover (optional, defaults to no)
| $3 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $4 | Region

#### `rds_set_backup_policy()`
- Sets Backup Policy for specified Database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Backup Retention Period
| $3 | Preferred Backup Window
| $4 | Apply Immediately (yes/no, defaults to yes)
| $5 | Region

#### `rds_set_engine_version()`
- Sets Engine Version for specified Database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Engine Version
| $3 | Parameter Group (optional, only required if changing the Major version of the Engine)
| $4 | Apply Immediately (yes/no, defaults to yes)
| $5 | Region

#### `rds_set_maintenance_policy()`
- Sets Maintenance Policy for specified Database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Preferred Maintenance Window
| $3 | Auto Minor Version Upgrade (yes/no, defaults to no)
| $4 | Allow Major Version Upgrade (yes/no, defaults to no)
| $5 | Apply Immediately (yes/no, defaults to yes)
| $6 | Region

#### `rds_set_parameter_group()`
- Sets Parameter Group for specified Database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Parameter Group
| $3 | Apply Immediately (yes/no, defaults to yes)
| $4 | Region

#### `rds_set_security_group()`
- Sets Security Group for specified Database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | Security Group
| $3 | Apply Immediately (yes/no, defaults to yes)
| $4 | Region

#### `rds_update_password()`
- Updates Master Password for database

| Arg | Description |
| --- | --- |
| $1 | Database ID
| $2 | New Password
| $3 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $4 | Region
***
### aws/rds_mysql.inc
Collection of functions related to AWS (RDS - MySQL)

#### `load_info_mysql()`
- Loads MySQL Information into memory from CloudFormation Outputs file

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Outputs File
| $2 | Region

#### `mysql_create()`
- Creates MySQL Database

| Arg | Description |
| --- | --- |
| $1 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $2 | Region

#### `mysql_create_from_snapshot()`
- Creates MySQL Database from specified Snapshot

| Arg | Description |
| --- | --- |
| $1 | Snapshot ARN
| $2 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $3 | Region

#### `mysql_create_replica()`
- Creates MySQL Replica Database

| Arg | Description |
| --- | --- |
| $1 | Iteration
| $2 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $3 | Region

#### `mysql_execute_sql()`
- Attempts to connect to specified MySQL Database in order to execute SQL file

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Name (optional, if not specified, it ignores this option)
| $5 | Database Port (optional, if not specified, defaults to MYSQL_DEFAULT_PORT)
| $6 | SQL File

#### `mysql_execute_sql_output_to_file()`
- Attempts to connect to specified MySQL Database in order to execute SQL file and send its output to specified file

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Name (optional, if not specified, it ignores this option)
| $5 | Database Port (optional, if not specified, defaults to MYSQL_DEFAULT_PORT)
| $6 | SQL File
| $7 | Output File

#### `mysql_set_dns()`
- Sets DNS record for MySQL Database

| Arg | Description |
| --- | --- |
| $1 | Region

#### `mysql_verify_connection()`
- Attempts to connect to specified MySQL Database in order to verify if database is available and credentials are correct

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Port (optional, if not specified, defaults to MYSQL_DEFAULT_PORT)
***
### aws/rds_postgresql.inc
Collection of functions related to AWS (RDS - PostgreSQL)

#### `load_info_postgresql()`
- Loads PostgreSQL Information into memory from CloudFormation Outputs file

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Outputs File
| $2 | Region

#### `postgresql_create()`
- Creates PostgreSQL Database

| Arg | Description |
| --- | --- |
| $1 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $2 | Region

#### `postgresql_create_from_snapshot()`
- Creates PostgreSQL Database from specified Snapshot

| Arg | Description |
| --- | --- |
| $1 | Snapshot ARN
| $2 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $3 | Region

#### `postgresql_create_replica()`
- Creates PostgreSQL Replica Database

| Arg | Description |
| --- | --- |
| $1 | Iteration
| $2 | Verification Timeout &lt;in minutes, defaults to 30 minutes&gt;
| $3 | Region

#### `postgresql_execute_sql()`
- Attempts to connect to specified PostgreSQL Database in order to execute SQL file

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Name (optional, defaults to 'postgres')
| $5 | Database Port (optional, if not specified, defaults to POSTGRESQL_DEFAULT_PORT)
| $6 | SQL File

#### `postgresql_set_dns()`
- Sets DNS record for PostgreSQL Database

| Arg | Description |
| --- | --- |
| $1 | Region

#### `postgresql_verify_connection()`
- Attempts to connect to specified PostgreSQL Database in order to verify if database is available and credentials are correct

| Arg | Description |
| --- | --- |
| $1 | Database Endpoint
| $2 | Username
| $3 | Password
| $4 | Database Name (optional, defaults to 'postgres')
| $5 | Database Port (optional, if not specified, defaults to POSTGRESQL_DEFAULT_PORT)
***
### aws/route53.inc
Collection of functions related to AWS (Route53)

#### `route53_delete_record()`
- Route53 | Performs Delete on Record

| Arg | Description |
| --- | --- |
| $1 | Zone ID
| $2 | Record Name
| $3 | Region

#### `route53_delete_record_call()`
- Route53 | Performs Delete on Record (the actual CLI call to perform delete)

| Arg | Description |
| --- | --- |
| $1 | Zone ID
| $2 | Record Name
| $3 | Record Type
| $4 | Record TTL
| $5 | Record Value
| $6 | Record Weight (optional, used for weighted records)
| $7 | Record Set Identifier (optional, used for weighted records)
| $8 | Region

#### `route53_get_record()`
- Retrieves current Record Information and returns value list as an Array Set

| Arg | Description |
| --- | --- |
| $1 | Array name to pass info to
| $2 | Zone ID
| $3 | Record Name
| $4 | Region

#### `route53_upsert_record()`
- Route53 | Performs Upsert on Record

| Arg | Description |
| --- | --- |
| $1 | Zone ID
| $2 | Record Name
| $3 | Record Type
| $4 | Record TTL (optional, defaults to AWS_ROUTE53_DEFAULT_TTL)
| $5 | Record Value (semicolon delimited)
| $6 | Record Weight (optional, used for weighted records)
| $7 | Record Set Identifier (optional, used for weighted records)
| $8 | Region
***
### aws/s3.inc
Collection of functions related to AWS (S3)

#### `s3_cp_download()`
- Downloads files from AWS S3 Bucket (via copy method)

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 Target
| $3 | Local File

#### `s3_cp_upload()`
- Uploads files to AWS S3 Bucket (via copy method)

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 Target
| $3 | Local File
| $4 | Enable Server-side encryption (optional, defaults to yes)
| $5 | Enable Reduced Redundancy (optional, defaults to yes)

#### `s3_delete()`
- Downloads files from AWS S3 Bucket (via copy method)

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 Target

#### `s3_move_in_bucket()`
- Moves files inside AWS S3 Bucket

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 File Source
| $3 | S3 File Target
| $4 | Enable Server-side encryption (optional, defaults to yes)
| $5 | Enable Reduced Redundancy (optional, defaults to yes)

#### `s3_sync_download()`
- Downloads files from AWS S3 Bucket (via sync method)

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 Directory
| $3 | Local Directory
| $4 | Delete (yes/no) - Files that exist in the destination but not in the source are deleted during sync (optional, defaults to no)
| $5 | Exact Timestamps (yes/no) - When syncing from S3 to local,  same-sized items  will  be  ignored  only  when  the timestamps match
| $6 | Excludes (optional, semicolon separated)
| $7 | Include (optional, semicolon separated)

#### `s3_sync_upload()`
- Uploads files to AWS S3 Bucket (via sync method)

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 Directory
| $3 | Local Directory
| $4 | Delete (yes/no) - Files that exist in the destination but not in the source are deleted during sync (optional, defaults to no)
| $5 | Exact Timestamps (yes/no) - When syncing from S3 to local, same-sized items  will  be  ignored  only  when  the timestamps match
| $6 | Enable Server-side encryption (optional, defaults to yes)
| $7 | Enable Reduced Redundancy (optional, defaults to yes)
| $8 | Excludes (optional, semicolon separated)
| $9 | Include (optional, semicolon separated)

#### `s3_verify()`
- Verifies connectivity to specified S3 bucket

| Arg | Description |
| --- | --- |
| $1 | S3 Bucket
| $2 | S3 Directory (optional, if left empty it will attempt to check against the root of the bucket)
| $3 | Display Output (optional, yes/no, defaults to no)
***
### aws/s3_website.inc
Collection of functions related to S3 Site

#### `load_info_s3_website()`
- Loads S3 Website Information into memory from CloudFormation Outputs file

| Arg | Description |
| --- | --- |
| $1 | CloudFormation Outputs File

#### `process_s3_website()`
- Processes S3 Website

| Arg | Description |
| --- | --- |
| $1 | s3_website.yaml base
| $2 | Static site files directory
| $3 | Force Sync (optional, defaults to no)
| $4 | Region
***
### aws/ssl.inc
Collection of functions related to SSL Certificates in AWS

#### `acm_certificate_exists()`
- Checks to see if SSL Certificate exists in ACM

| Arg | Description |
| --- | --- |
| $1 | SSL Certificate Name
| $2 | Region

#### `acm_import_certificate()`
- Uploads SSL Certificate to ACM (AWS Certificate Manager)

| Arg | Description |
| --- | --- |
| $1 | SSL Certificate Name
| $2 | SSL CRT File
| $3 | SSL Chain File
| $4 | SSL Key File
| $5 | Region

#### `acm_tag_certificate()`
- Tags SSL Certificate in ACM (AWS Certificate Manager)

| Arg | Description |
| --- | --- |
| $1 | SSL Certificate Name
| $2 | SSL Certificate ARN
| $3 | Region

#### `iam_certificate_exists()`
- Checks to see if SSL Certificate exists in IAM

| Arg | Description |
| --- | --- |
| $1 | SSL Certificate Name
| $2 | Region

#### `iam_import_certificate()`
- Uploads SSL Certificate to IAM

| Arg | Description |
| --- | --- |
| $1 | SSL Certificate Name
| $2 | SSL CRT File
| $3 | SSL Chain File
| $4 | SSL Key File
| $5 | Region

#### `ssl_download_certificate()`
- Downloads specified SSL Certificate from AWS
- Generates the following files:
| Certificate | DIRECTORY_SSL/server.crt
| Chain |       DIRECTORY_SSL/server.chain
| Key |         DIRECTORY_SSL/server.key
| Combined |    DIRECTORY_SSL/server.combined

| Arg | Description |
| --- | --- |
| $1 | SSL Certificate Name
| $2 | Common Name (optional, if specified, certificate will be placed in a sub directory named after this Common Name)
| $3 | Region (optional, if not specified, Region will be loaded through AWS Metadata
***
### aws/ssm.inc
Collection of functions related to SSM (AWS Systems Manager)

#### `parameter_delete()`
- Deletes SSM Parameter

| Arg | Description |
| --- | --- |
| $1 | Parameter
| $2 | Skip Exists Check (optional, defaults to no)
| $3 | Region

#### `parameter_exists()`
- Returns true if parameter exists

| Arg | Description |
| --- | --- |
| $1 | Parameter
| $2 | Region

#### `parameter_get()`
- Retrieves SSM Parameter from AWS and passes it to specified variable
| $1 | Variable name to pass Parameter to
| $2 | Parameter
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameter_get_file()`
- Retrieves SSM Parameter from AWS to generate specified file
| $1 | File to create
| $2 | Parameter
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameter_get_file_multi_part()`
- Retrieves SSM Parameter from AWS to generate specified file
| $1 | File to create
| $2 | Parameter
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameter_get_file_silent()`
- Retrieves SSM Parameter from AWS to generate specified file (Does not output to screen unless there is an error)
| $1 | File to create
| $2 | Parameter
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameter_get_silent()`
- Retrieves SSM Parameter from AWS and passes it to specified variable (Does not output to screen unless there is an error)
| $1 | Variable name to pass Parameter to
| $2 | Parameter
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameter_path_delete()`
- Dumps parameters to file in [key]=value format

| Arg | Description |
| --- | --- |
| $1 | Parameter Path
| $2 | Region

#### `parameter_put()`
- Creates SSM Parameter
- Parameter values that end in an asterisk (*) will have the asterisk removed and its data masked for any visible output

| Arg | Description |
| --- | --- |
| $1 | Parameter
| $2 | Value
| $3 | Encryption Key [optional, defaults to AWS Account Default key]
| $4 | Description [optional]
| $5 | Replace Parameter if exists [yes/no, defaults to yes]
| $6 | Region

#### `parameter_put_file()`
- Creates SSM Parameter from a file (base64 encoded)

| Arg | Description |
| --- | --- |
| $1 | Parameter
| $2 | Filename
| $3 | Encryption Key [optional, defaults to AWS Account Default key]
| $4 | Description [optional]
| $5 | Replace Parameter if exists [yes/no, defaults to yes]
| $6 | Region

#### `parameter_put_file_multi_part()`
- Creates SSM Parameter from a file (base64 encoded)
- Separates file into 4096 bit chunks

| Arg | Description |
| --- | --- |
| $1 | Parameter
| $2 | Filename
| $3 | Encryption Key [optional, defaults to AWS Account Default key]
| $4 | Description [optional]
| $5 | Replace Parameter if exists [yes/no, defaults to yes]
| $6 | Region

#### `parameter_tag()`
- Tags SSM Parameter
- Parameter values that end in an asterisk (*) will have the asterisk removed and its data masked for any visible output

| Arg | Description |
| --- | --- |
| $1 | Parameter
| $2 | Tags [semicolon separated, values are sent as &lt;KeyName&gt;:&lt;Value&gt;;&lt;KeyName&gt;:&lt;Value&gt;]
| $3 | Region

#### `parameter_tag_path()`
- Tags SSM Parameter Path

| Arg | Description |
| --- | --- |
| $1 | Parameter Path
| $2 | Tags [semicolon separated, values are sent as &lt;KeyName&gt;:&lt;Value&gt;;&lt;KeyName&gt;:&lt;Value&gt;]
| $3 | Region

#### `parameters_to_key_value_file()`
- Dumps parameters to file in [key]=value format

| Arg | Description |
| --- | --- |
| $1 | Parameter Path
| $2 | Output file
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameters_to_key_value_file_multipart()`
- Dumps parameters to file in [key]=value format (for Multipart Downloads)

| Arg | Description |
| --- | --- |
| $1 | Parameter Path
| $2 | Output file
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region

#### `parameters_to_properties_file()`
- Dumps parameters to file in key=value format

| Arg | Description |
| --- | --- |
| $1 | Parameter Path
| $2 | Output file
| $3 | Use Crypt functionality (encrypt/decrypt) [yes/no, defaults to yes]
| $4 | Region
***
### aws/vpc.inc
Collection of functions related to AWS (VPC)

#### `vpc_peering_connection_enable_dns()`
- Enables DNS Resolution between VPCs for VPC Peering Connection

| Arg | Description |
| --- | --- |
| $1 | Resource ID
| $2 | Region
***
### core/builder.inc
Collection of functions related to Builder

#### `display_variables_object_lists()`
- Displays Object Lists variables

#### `display_variables_parameter()`
- Displays Parameter variables

#### `display_variables_project()`
- Displays Project variables

#### `generate_file_details()`
- Generates Details file

| Arg | Description |
| --- | --- |
| $1 | Output file

#### `generate_file_parameters()`
- Generates Parameters file

| Arg | Description |
| --- | --- |
| $1 | Output file

#### `generate_file_tags()`
- Generates Tags file

| Arg | Description |
| --- | --- |
| $1 | Output file

#### `generate_file_targets()`
- Generates CloudFormation Targets file

| Arg | Description |
| --- | --- |
| $1 | Output file

#### `generate_file_template()`
- Generates CloudFormation Template file

| Arg | Description |
| --- | --- |
| $1 | Output file

#### `load_object_files()`
- Loads Files from ARRAY_OBJECTS_&lt;type&gt; into Corresponding Files Array

| Arg | Description |
| --- | --- |
| $1 | Object Type (conditions / mappings / parameters / outputs / resources)

#### `load_objects()`
- Loads Object Names into Corresponding Objects Array

| Arg | Description |
| --- | --- |
| $1 | Object Type (conditions / mappings / parameters / outputs / resources)
| $2 | Object Source File

#### `load_objects_base()`
- Loads Object Names into Corresponding Objects Array (using basename of object)

| Arg | Description |
| --- | --- |
| $1 | Object Type (parameters)
| $2 | Object Source File

#### `load_paradigm()`
- Loads Paradigm Model into memory

#### `load_parameters()`
- Loads Parameters into memory from Projects file

| Arg | Description |
| --- | --- |
| $1 | Project File

#### `load_project()`
- loads Project Information

| Arg | Description |
| --- | --- |
| $1 | Project File

#### `load_project_sam()`
- loads SAM Project Information

| Arg | Description |
| --- | --- |
| $1 | Project File
| $2 | SAM File

#### `load_secrets()`
- loads Secrets Information

| Arg | Description |
| --- | --- |
| $1 | Project

#### `load_targets()`
- Loads Targets to Targets Object Array

| Arg | Description |
| --- | --- |
| $1 | Target Source File

#### `load_templates()`
- Loads Objects | Templates to CFN Object Array

| Arg | Description |
| --- | --- |
| $1 | Object Source File

#### `manifest_get_checksum()`
- Retrieves Checksum of specified Bulder Object from the Manifest DynamoDB Table

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Stack ID
| $3 | DynamoDB Table (Manifest)
| $4 | Region

#### `manifest_put_checksum()`
- Puts Checksum of specified Bulder Object in the Manifest DynamoDB Table

| Arg | Description |
| --- | --- |
| $1 | Stack ID
| $2 | Checksum
| $3 | DynamoDB Table (Manifest)
| $4 | Region
***
### core/color.inc
Collection of functions related to bringing some color to STDOUT

#### `color_lookup()`
- wrapper function for making curl calls
- performs retries if enabled and appropriate
- attempts to catch all curl specific return codes and attempts to handle the return code appropriately

| Arg | Description |
| --- | --- |
| $1 | color

#### `color_text()`
- returns text gently swaddled in radiant color

| Arg | Description |
| --- | --- |
| $1 | color
| $2 | text
| $3 | color to resume

#### `debug_color_text()`
- echos to screen a test line with each of the defined colors
- for debug purposes only
***
### core/common.inc
Collection of common functions

#### `add_element_to_array()`

| Arg | Description |
| --- | --- |
| $1 | Array Name
| $2 | Element

#### `base64_decode()`
- Decodes a string to base64

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | encoded string

#### `base64_encode()`
- Encodes a string to base64

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | string

#### `base64_encode_and_compress()`
- Encodes a string to base64 and compresses it (using gzip -9)

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | string

#### `call_sleep()`
- used to call sleep

| Arg | Description |
| --- | --- |
| $1 | Number of seconds to sleep
| $2 | Reason (optional)

#### `call_sleep_random()`
- used to call sleep, will randomize the amount of seconds based on Max sleep time

| Arg | Description |
| --- | --- |
| $1 | Max sleep time (optional, defaults to 60 seconds if not specified)

#### `does_array_contain_element()`

| Arg | Description |
| --- | --- |
| $1 | Element string to search for
| $2 | Array (pass this way | "${array[@]}")

#### `echo_key_value()`
- Returns Key Value line used in variable output files

| Arg | Description |
| --- | --- |
| $1 | key
| $2 | value

#### `explode_variables_to_file()`

| Arg | Description |
| --- | --- |
| $1 | File to write to
| $2 | Delimited Variable

#### `file_update_owner()`

| Arg | Description |
| --- | --- |
| $1 | file
| $2 | owner (example | apache:apache)

#### `file_update_permissions()`

| Arg | Description |
| --- | --- |
| $1 | file
| $2 | owner (example | 0400)

#### `filesize_bytes_to_human_readable()`
- converts bytes into a human readable format (GB / MB / KB)

| Arg | Description |
| --- | --- |
| $1 | filesize in bytes (as integer)

#### `function_exists()`
- Returns true if specified function name exists, otherwise false

| Arg | Description |
| --- | --- |
| $1 | Function name

#### `generate_temp_directory()`
- creates a temporary directory and returns the name of said directory to the passed variable name

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Purpose string (optional)
| $3 | Base Directory override (optional)

#### `generate_temp_file()`
- creates a temporary file and returns the name of said file to the passed variable name

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to
| $2 | Purpose string (optional)
| $3 | Directory override (optional)

#### `generate_uuid()`
- Creates a UUID (Removes hyphens from normal UUID format)

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass output to (optional)
| $2 | String Length (optional)

#### `html_escape()`
- HTML Escapes string

| Arg | Description |
| --- | --- |
| $1 | String

#### `is_empty()`
- Checks if a variable is set (not null) and has a value.

| Arg | Description |
| --- | --- |
| $1 | Variable passed to check
| $2 | Strict Checking - will return true if the value exists but is zero (optional, defaults to yes)

#### `is_int()`
- Checks if a variable an integer

| Arg | Description |
| --- | --- |
| $1 | Variable passed to check

#### `is_server_up()`
- performs ping test against specified server to determine if server is up

| Arg | Description |
| --- | --- |
| $1 | Target server address

#### `json_safe()`
- strips out or replaces characters that would mess up the JSON structure

| Arg | Description |
| --- | --- |
| $1 | string to json_safe

#### `load_array_key_values_from_file()`
- loads Key values passed via an array from specified file

| Arg | Description |
| --- | --- |
| $1 | array
| $2 | file to load from
| $3 | Prepend String (optional, if supplied automatically adds an Underscore)
| $4 | Parse String (optional)

#### `load_array_key_values_from_yaml_file()`
- loads Key values passed via an array from specified YAML file

| Arg | Description |
| --- | --- |
| $1 | array
| $2 | file to load from
| $3 | Prepend String (optional, if supplied automatically adds an Underscore)

#### `load_array_properties_from_file()`
- loads properties variables passed via an array from specified file

| Arg | Description |
| --- | --- |
| $1 | array
| $2 | file to load from
| $3 | Prepend String (optional, if supplied automatically adds an Underscore)

#### `load_key_value_from_file()`
- Loads key value from file

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to (optional, if not specified outputs String to STDOUT)
| $2 | Key name
| $3 | Filename
| $4 | Parse String (optional)

#### `load_property_from_file()`
- Loads property from file

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to (optional, if not specified outputs String to STDOUT)
| $2 | Key name
| $3 | Filename

#### `option_enabled()`
- Utilizes yes/no flag for variable. Returns true if 'yes'
- Usage | "if option_enabled VERBOSE; then"  - NOTE | the argument is the variable name, not the variable (no $)

| Arg | Description |
| --- | --- |
| $1 | Variable passed to check

#### `return_file_last_modify_timestamp()`
- returns last modify timestamp of specified file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_file_md5sum()`
- returns md5 checksum of file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_file_mime()`
- returns MIME type of file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_file_sha1sum()`
- returns sha1 checksum of file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_file_sha256sum()`
- returns sha256 checksum of file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_filesize_of_file()`
- returns filesize (in human-readable format) of specified file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_filesize_of_file_in_bytes()`
- returns filesize (in bytes) of specified file

| Arg | Description |
| --- | --- |
| $1 | filename
| $2 | Variable name to pass info to (optional)

#### `return_header_string()`
- Returns Header string information from comments of file
- This keys off of the '# &lt;header_identifier&gt; | ' comment line found at the top of the scripts

| Arg | Description |
| --- | --- |
| $1 | Header Identifier
| $2 | File to inspect
| $3 | Variable name to pass info to (optional)

#### `return_parameter_string()`
- Returns Parameter string information from comments of file
- This keys off of the '## &lt;parameter_identifier&gt; | ' comment line found inside the file

| Arg | Description |
| --- | --- |
| $1 | Parameter Identifier
| $2 | File to inspect
| $3 | Variable name to pass info to (optional)

#### `return_yaml_string()`
- Returns YAML string information from file

| Arg | Description |
| --- | --- |
| $1 | Variable name to pass info to (optional, if not specified outputs String to STDOUT)
| $2 | String Identifier (Header)
| $3 | File to inspect

#### `source_include()`
- Loads specified file as source and logs it

| Arg | Description |
| --- | --- |
| $1 | Source file

#### `sync_disks()`
- Performs sync and logs it as an entry

#### `time_elapsed()`
- calculates the amount of time elapsed between two timestamps
- this function uses timestamps with microsecond precision

| Arg | Description |
| --- | --- |
| $1 | Time Start (required)
| $2 | Time End (optional, if not supplied it will calculate based on the current time)

#### `timestamp_to_human_readable()`
- converts UNIX timestamp to human readable format
- trims UNIX timestamp down to 10 characters (this allows us to pass extended timestamps (UNIX+microseconds) and not have an error

| Arg | Description |
| --- | --- |
| $1 | UNIX timestamp

#### `to_lower()`
- Converts string to lowercase

| Arg | Description |
| --- | --- |
| $1 | string to convert

#### `to_upper()`
- Converts string to uppercase

| Arg | Description |
| --- | --- |
| $1 | string to convert

#### `trim()`
- trims whitespace from left and right side of supplied string

| Arg | Description |
| --- | --- |
| $1 | string to trim

#### `uptime_greater_than()`
- Utilizes yes/no flag for variable. Returns true if 'yes'

| Arg | Description |
| --- | --- |
| $1 | Target uptime in seconds

#### `url_decode()`
- URL Decodes string

| Arg | Description |
| --- | --- |
| $1 | String

#### `url_encode()`
- URL Encodes string

| Arg | Description |
| --- | --- |
| $1 | String

#### `verify_array_key_values()`
- verifies Key values passed via an array are loaded into memory and are not empty

| Arg | Description |
| --- | --- |
| $1 | array
| $2 | Prepend String (optional, if supplied automatically adds an Underscore)

#### `version_to_integer()`
- verifies Key values passed via an array are loaded into memory and are not empty

| Arg | Description |
| --- | --- |
| $1 | Version String
| $2 | Variable name to pass output to (optional)

#### `write_key_value_to_file()`
- Writes [key]=value line to file

| Arg | Description |
| --- | --- |
| $1  | File to write to
| $2  | Variable Name
| $3  | Variable Value
***
### core/logging.inc
Collection of logging related functions

#### `line_break()`
- prints to screen a line that spans across the full width (columns) of the open terminal
- serves no purpose other than to be a pretty, pretty princess

#### `log()`
- echo log entry to screen

| Arg | Description |
| --- | --- |
| $1 | entry string
| $2 | Message Type
| $3 | Message Content Color Override

#### `log_add_from_file()`
- echo content of a file  to screen

| Arg | Description |
| --- | --- |
| $1 | file containing data to add to log
| $2 | string description of contents
| $3 | number of lines to display (optional, defaults to 99999)

#### `log_error()`
- echo log error entry to screen
- this function prepends "ERROR:" to the beginning of the log entry to ensure consistency in the logs

| Arg | Description |
| --- | --- |
| $1 | entry string
| $2 | Message Content Color Override

#### `log_file_content()`
- logs file content

| Arg | Description |
| --- | --- |
| $1 | Line Number
| $2 | Entry string

#### `log_highlight()`
- echo log highlight entry to screen

| Arg | Description |
| --- | --- |
| $1 | entry string

#### `log_load_color_child()`
- loads color palette for CHILD type scripts

#### `log_load_color_parent()`
- loads color palette for PARENT type scripts

#### `log_notice()`
- echo log notice entry to screen
- this function prepends "NOTICE:" to the beginning of the log entry to ensure consistency in the logs

| Arg | Description |
| --- | --- |
| $1 | entry string
| $2 | Message Content Color Override

#### `log_quiet()`
- suppresses echoing log entry to screen

| Arg | Description |
| --- | --- |
| $1 | entry string

#### `log_success()`
- echo log success entry to screen
- this function prepends "SUCCESS:" to the beginning of the log entry to ensure consistency in the logs

| Arg | Description |
| --- | --- |
| $1 | entry string
| $2 | Message Content Color Override

#### `log_warning()`
- echo log warning entry to screen
- this function prepends "WARNING:" to the beginning of the log entry to ensure consistency in the logs

| Arg | Description |
| --- | --- |
| $1 | entry string
| $2 | Message Content Color Override

#### `verbose_display_date()`
- truncates log date timestamp to HH:MM:SS only

| Arg | Description |
| --- | --- |
| $1 | timestamp
***
### core/script_logic.inc
Collection of Script Logic related functions.

#### `cleanup()`
- attempts to perform cleanup
- removes temporary files as specified in array TEMPORARY_FILES
- this will only remove files in a recognized temp directory, otherwise it issues a warning and skips the file
- performs disk sync once finished

#### `exec_script()`
- handles logic for calling a script from within a script (inception?)
- returns the exit status of the script which was ran

| Arg | Description |
| --- | --- |
| $1 | script to execute (use variables as supplied in reference_script_definitions.inc)
| $2 | arguments to pass to script

#### `exit_logic()`
- handles exit logic
- attempts to run all required cleanup

| Arg | Description |
| --- | --- |
| $1 | Exit code to return
| $2 | Exit message (optional)

#### `exit_logic_skip_cleanup()`
- handles exit logic
- skips cleanup

| Arg | Description |
| --- | --- |
| $1 | Exit code to return
| $2 | Exit message (optional)

#### `exit_trap()`
- handles exit logic when a trap is caught
- attempts to run all required cleanup

| Arg | Description |
| --- | --- |
| $1 | Exit code to return
| $2 | Exit message (optional)

#### `exit_trap_ensure_graceful()`
- handles exit logic when trap EXIT is caught
- attempts to run all required cleanup
- this is a safety feature in case a graceful exit is not already defined in the script (ie | [exit_logic 0] not included as the last line of a script)

#### `lookup_exit_code()`
- Returns human-friendly string information based on exit code integer (will return 'UNKNOWN' if it is unable to parse exitcode from file)

| Arg | Description |
| --- | --- |
| $1 | Exit Code
| $2 | File containing exit codes, optional (if not supplied, we will use the default REFERENCE_EXIT_CODES

#### `return_script_purpose()`
- Returns version number of the selected script ($1)
- This keys off of the '# purpose | ' comment line found at the top of the scripts
- Normally called in self-reference [via | $(return_script_version "${0}")]

| Arg | Description |
| --- | --- |
| $1 | File to inspect

#### `return_script_version()`
- Returns version number of the selected script ($1)
- This keys off of the '# version | ' comment line found at the top of the scripts
- Normally called in self-reference [via | $(return_script_version "${0}")]

| Arg | Description |
| --- | --- |
| $1 | File to inspect

#### `sort_array_required_executeables()`
- sorts / removes duplicates in the REQUIRED_EXECUTABLES global array

#### `sort_array_required_source_files()`
- sorts / removes duplicates in the REQUIRED_SOURCE_FILES global array

#### `start_logic()`
- handles all script startup logic
- reference code handling
- log initialization
- loads required source files (as defined in array REQUIRED_SOURCE_FILES)
- verifies dependencies (as defined in array REQUIRED_EXECUTABLES)
- should be called right after argument collection in every script

#### `usage()`
- Displays help / usage

#### `usage_banner()`
- Displays Usage Banner (Details, Version info, etc)

| Arg | Description |
| --- | --- |
| $1 | File to inspect

#### `usage_normalized_arguments()`
- displays argument list which is common for all scripts

#### `verify_dependencies()`
- validates that all required executables for the script to run are present (and accessible via PATH)
- based on elements from REQUIRED_EXECUTABLES array
