import boto3


def lambda_handler(event, context):

   client = boto3.resource('dynamodb')

   table = client.Table('sensordata')

   response = table.put_item(

       Item={
          
            'id': event['id'],
           
           'temperature': event['temperature'],
           
           'status': event['status'],
           
           'location': event['location'],
           
           'datetime': event['datetime']
       }
   )

   return {

       'statusCode': response['ResponseMetadata']['HTTPStatusCode'],

       'body': 'Record ' + event['id'] + ' added'

   }