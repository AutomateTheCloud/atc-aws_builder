###------------------------------------------------------------------------------------------------
# script:  definitions.inc
# purpose: Declaration of library source files to be used to ensure consistency.
# version: 1.0.0
###------------------------------------------------------------------------------------------------

###------------------------------------------------------------------------------------------------
## Configuration files
declare LIB_CONFIG_GLOBAL="${DIRECTORY_BASE}/config/global.config"

###------------------------------------------------------------------------------------------------
## Directories
declare DIRECTORY_CONFIG="${DIRECTORY_BASE}/config"
declare DIRECTORY_LIB="${DIRECTORY_BASE}/lib"
declare DIRECTORY_LIB_DEPENDENCIES="${DIRECTORY_LIB}/dependencies"
declare DIRECTORY_LIB_FUNCTIONS="${DIRECTORY_LIB}/functions"
declare DIRECTORY_LIB_REFERENCE="${DIRECTORY_LIB}/reference"
declare DIRECTORY_SCRIPTS="${DIRECTORY_BASE}/scripts"

###------------------------------------------------------------------------------------------------
## Distribution Files
declare DISTRIBUTION_README="${DIRECTORY_BASE}/README.md"
declare DISTRIBUTION_LICENSE="${DIRECTORY_BASE}/LICENSE.md"

###------------------------------------------------------------------------------------------------
## Reference Files
declare LIB_REFERENCE_DEFINITIONS="${DIRECTORY_BASE}/lib/reference/definitions.inc"
declare LIB_REFERENCE_DETAILS="${DIRECTORY_BASE}/lib/reference/details.inc"
declare LIB_REFERENCE_EXIT_CODES="${DIRECTORY_BASE}/lib/reference/exit_codes.inc"

###------------------------------------------------------------------------------------------------
## Function: AWS
declare LIB_FUNCTIONS_AWS_AMI="${DIRECTORY_BASE}/lib/functions/aws/ami.inc"
declare LIB_FUNCTIONS_AWS_CLOUDFORMATION="${DIRECTORY_BASE}/lib/functions/aws/cloudformation.inc"
declare LIB_FUNCTIONS_AWS_CLOUDFRONT="${DIRECTORY_BASE}/lib/functions/aws/cloudfront.inc"
declare LIB_FUNCTIONS_AWS_CODEDEPLOY="${DIRECTORY_BASE}/lib/functions/aws/codedeploy.inc"
declare LIB_FUNCTIONS_AWS_EC2="${DIRECTORY_BASE}/lib/functions/aws/ec2.inc"
declare LIB_FUNCTIONS_AWS_ELASTICACHE_MEMCACHED="${DIRECTORY_BASE}/lib/functions/aws/elasticache_memcached.inc"
declare LIB_FUNCTIONS_AWS_ELASTICACHE_REDIS="${DIRECTORY_BASE}/lib/functions/aws/elasticache_redis.inc"
declare LIB_FUNCTIONS_AWS_ENI="${DIRECTORY_BASE}/lib/functions/aws/eni.inc"
declare LIB_FUNCTIONS_AWS_IAM="${DIRECTORY_BASE}/lib/functions/aws/iam.inc"
declare LIB_FUNCTIONS_AWS_INITIALIZE="${DIRECTORY_BASE}/lib/functions/aws/initialize.inc"
declare LIB_FUNCTIONS_AWS_KMS="${DIRECTORY_BASE}/lib/functions/aws/kms.inc"
declare LIB_FUNCTIONS_AWS_LAMBDA="${DIRECTORY_BASE}/lib/functions/aws/lambda.inc"
declare LIB_FUNCTIONS_AWS_LOADBALANCER="${DIRECTORY_BASE}/lib/functions/aws/loadbalancer.inc"
declare LIB_FUNCTIONS_AWS_METADATA="${DIRECTORY_BASE}/lib/functions/aws/metadata.inc"
declare LIB_FUNCTIONS_AWS_RDS="${DIRECTORY_BASE}/lib/functions/aws/rds.inc"
declare LIB_FUNCTIONS_AWS_RDS_AURORA_POSTGRESQL="${DIRECTORY_BASE}/lib/functions/aws/rds_aurora_postgresql.inc"
declare LIB_FUNCTIONS_AWS_RDS_MYSQL="${DIRECTORY_BASE}/lib/functions/aws/rds_mysql.inc"
declare LIB_FUNCTIONS_AWS_RDS_POSTGRESQL="${DIRECTORY_BASE}/lib/functions/aws/rds_postgresql.inc"
declare LIB_FUNCTIONS_AWS_ROUTE53="${DIRECTORY_BASE}/lib/functions/aws/route53.inc"
declare LIB_FUNCTIONS_AWS_S3="${DIRECTORY_BASE}/lib/functions/aws/s3.inc"
declare LIB_FUNCTIONS_AWS_S3_WEBSITE="${DIRECTORY_BASE}/lib/functions/aws/s3_website.inc"
declare LIB_FUNCTIONS_AWS_SSM="${DIRECTORY_BASE}/lib/functions/aws/ssm.inc"
declare LIB_FUNCTIONS_AWS_SSL="${DIRECTORY_BASE}/lib/functions/aws/ssl.inc"
declare LIB_FUNCTIONS_AWS_VPC="${DIRECTORY_BASE}/lib/functions/aws/vpc.inc"

###------------------------------------------------------------------------------------------------
## Function: Core
declare LIB_FUNCTIONS_CORE_BUILDER="${DIRECTORY_BASE}/lib/functions/core/builder.inc"
declare LIB_FUNCTIONS_CORE_COLOR="${DIRECTORY_BASE}/lib/functions/core/color.inc"
declare LIB_FUNCTIONS_CORE_COMMON="${DIRECTORY_BASE}/lib/functions/core/common.inc"
declare LIB_FUNCTIONS_CORE_LOGGING="${DIRECTORY_BASE}/lib/functions/core/logging.inc"
declare LIB_FUNCTIONS_CORE_SCRIPT_LOGIC="${DIRECTORY_BASE}/lib/functions/core/script_logic.inc"

###------------------------------------------------------------------------------------------------
## Scripts: AMI
declare SCRIPTS_AMI_AMAZON_LINUX2_ENCRYPTED="${DIRECTORY_BASE}/scripts/ami/amazon_linux2_encrypted.sh"

###------------------------------------------------------------------------------------------------
## Scripts: CloudFormation
declare SCRIPTS_CLOUDFORMATION_BUILD="${DIRECTORY_BASE}/scripts/cloudformation/build.sh"
declare SCRIPTS_CLOUDFORMATION_DEPLOY="${DIRECTORY_BASE}/scripts/cloudformation/deploy.sh"
declare SCRIPTS_CLOUDFORMATION_PROCESS="${DIRECTORY_BASE}/scripts/cloudformation/process.sh"
declare SCRIPTS_CLOUDFORMATION_TARGETS="${DIRECTORY_BASE}/scripts/cloudformation/targets.sh"
declare SCRIPTS_CLOUDFORMATION_VALIDATE="${DIRECTORY_BASE}/scripts/cloudformation/validate.sh"

###------------------------------------------------------------------------------------------------
## Scripts: CloudFront
declare SCRIPTS_CLOUDFRONT_INVALIDATE_CACHE="${DIRECTORY_BASE}/scripts/cloudfront/invalidate_cache.sh"

###------------------------------------------------------------------------------------------------
## Scripts: CodeDeploy
declare SCRIPTS_CODEDEPLOY_DEPLOY_REVISION="${DIRECTORY_BASE}/scripts/codedeploy/deploy_revision.sh"
declare SCRIPTS_CODEDEPLOY_REGISTER_REVISION="${DIRECTORY_BASE}/scripts/codedeploy/register_revision.sh"

###------------------------------------------------------------------------------------------------
## Scripts: EC2
declare SCRIPTS_EC2_GENERATE_SSH_KEYPAIR="${DIRECTORY_BASE}/scripts/ec2/generate_ssh_keypair.sh"

###------------------------------------------------------------------------------------------------
## Scripts: ElastiCache - Redis
declare SCRIPTS_ELASTICACHE_REDIS_CREATE="${DIRECTORY_BASE}/scripts/elasticache/redis/create.sh"

###------------------------------------------------------------------------------------------------
## Scripts: KMS
declare SCRIPTS_KMS_DECRYPT_FILE="${DIRECTORY_BASE}/scripts/kms/decrypt_file.sh"
declare SCRIPTS_KMS_ENCRYPT_FILE="${DIRECTORY_BASE}/scripts/kms/encrypt_file.sh"

###------------------------------------------------------------------------------------------------
## Scripts: RDS - Aurora PostgreSQL
declare SCRIPTS_RDS_AURORAPOSTGRESQL_CREATE="${DIRECTORY_BASE}/scripts/rds/aurorapostgresql/create.sh"

###------------------------------------------------------------------------------------------------
## Scripts: RDS - MySQL
declare SCRIPTS_RDS_MYSQL_CREATE="${DIRECTORY_BASE}/scripts/rds/mysql/create.sh"

###------------------------------------------------------------------------------------------------
## Scripts: RDS - PostgreSQL
declare SCRIPTS_RDS_POSTGRESQL_CREATE="${DIRECTORY_BASE}/scripts/rds/postgresql/create.sh"

###------------------------------------------------------------------------------------------------
## Scripts: RDS - Tools
declare SCRIPTS_RDS_TOOLS_DOWNLOAD_LOGS="${DIRECTORY_BASE}/scripts/rds/tools/download_logs.sh"
declare SCRIPTS_RDS_TOOLS_SYNC_ALL_LOGS_TO_S3="${DIRECTORY_BASE}/scripts/rds/tools/sync_all_logs_to_s3.sh"

###------------------------------------------------------------------------------------------------
## Scripts: S3
declare SCRIPTS_S3_DOWNLOAD_FILE="${DIRECTORY_BASE}/scripts/s3/download_file.sh"
declare SCRIPTS_S3_MOVE_FILE="${DIRECTORY_BASE}/scripts/s3/move_file.sh"
declare SCRIPTS_S3_UPLOAD_FILE="${DIRECTORY_BASE}/scripts/s3/upload_file.sh"
declare SCRIPTS_S3_UPLOAD_WEBSITE="${DIRECTORY_BASE}/scripts/s3/upload_website.sh"

###------------------------------------------------------------------------------------------------
## Scripts: SAM
declare SCRIPTS_SAM_BUILD="${DIRECTORY_BASE}/scripts/sam/build.sh"

###------------------------------------------------------------------------------------------------
## Scripts: Secrets
declare SCRIPTS_SECRETS_DEPLOY="${DIRECTORY_BASE}/scripts/secrets/deploy.sh"
declare SCRIPTS_SECRETS_PROCESS="${DIRECTORY_BASE}/scripts/secrets/process.sh"

###------------------------------------------------------------------------------------------------
## Scripts: SSL
declare SCRIPTS_SSL_UPLOAD_CERTIFICATE="${DIRECTORY_BASE}/scripts/ssl/upload_certificate.sh"

###------------------------------------------------------------------------------------------------
## Scripts: SSM
declare SCRIPTS_SSM_PARAMETER_DELETE="${DIRECTORY_BASE}/scripts/ssm/parameter_delete.sh"
declare SCRIPTS_SSM_PARAMETER_DELETE_PATH="${DIRECTORY_BASE}/scripts/ssm/parameter_delete_path.sh"
declare SCRIPTS_SSM_PARAMETER_GET="${DIRECTORY_BASE}/scripts/ssm/parameter_get.sh"
declare SCRIPTS_SSM_PARAMETER_GET_FILE="${DIRECTORY_BASE}/scripts/ssm/parameter_get_file.sh"
declare SCRIPTS_SSM_PARAMETER_GET_FILE_MULTI_PART="${DIRECTORY_BASE}/scripts/ssm/parameter_get_file_multi_part.sh"
declare SCRIPTS_SSM_PARAMETER_PUT="${DIRECTORY_BASE}/scripts/ssm/parameter_put.sh"
declare SCRIPTS_SSM_PARAMETER_PUT_FILE="${DIRECTORY_BASE}/scripts/ssm/parameter_put_file.sh"
declare SCRIPTS_SSM_PARAMETER_PUT_FILE_MULTI_PART="${DIRECTORY_BASE}/scripts/ssm/parameter_put_file_multi_part.sh"

###------------------------------------------------------------------------------------------------
## Scripts: Utility
declare SCRIPTS_UTILITY_README_GENERATOR="${DIRECTORY_BASE}/scripts/utility/readme_generator.sh"
declare SCRIPTS_UTILITY_INSTALL="${DIRECTORY_BASE}/scripts/utility/install.sh"
declare SCRIPTS_UTILITY_REPLACE_PLACEHOLDERS="${DIRECTORY_BASE}/scripts/utility/replace_placeholders.sh"
