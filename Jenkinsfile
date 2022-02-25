@Library('Utils') _

def imageName = 'srt_api'
def imageName2 = 'bbdd_srt_api'
def imageName3 = 'nginx_srt_api'
def dockerFile = 'Dockerfile.multistage'
def dockerFile2 = 'Dockerfile.bbdd'
def dockerFile3 = 'Dockerfile.nginx'

node('docker-builder')
{
    try{
        stage('Checkout SCM')
        {
            checkoutResults = checkout scm
            repoVersion = git.GetRepositoryVersionAndTagIt(checkoutResults)
        }
        if(BRANCH_NAME == "master" || BRANCH_NAME == "develop")
        {
            stage('Build APP Image')
            {
                dockerFactory.Build(imageName, "--no-cache -f ${env.WORKSPACE}/${dockerFile} ${env.WORKSPACE}")
            }
            stage('Build BBDD Image')
            {
                dockerFactory.Build(imageName2, "--no-cache -f ${env.WORKSPACE}/${dockerFile2} ${env.WORKSPACE}")
            }
            stage('Build NGINX Image')
            {
                dockerFactory.Build(imageName3, "--no-cache -f ${env.WORKSPACE}/${dockerFile3} ${env.WORKSPACE}")
            }
            stage('Push Images to registry')
            {
                if(BRANCH_NAME == "master")
                {
                    dockerFactory.PushWithTag(imageName, "${imageName}:${repoVersion}")
                    manager.addShortText("app:${repoVersion}")
                    dockerFactory.PushWithTag(imageName,"${imageName}:latest")

                    dockerFactory.PushWithTag(imageName2, "${imageName2}:${repoVersion}")
                    manager.addShortText("bbdd:${repoVersion}")
                    dockerFactory.PushWithTag(imageName2,"${imageName2}:latest")

                    dockerFactory.PushWithTag(imageName3, "${imageName3}:${repoVersion}")
                    manager.addShortText("nginx:${repoVersion}")
                    dockerFactory.PushWithTag(imageName3,"${imageName3}:latest")
                }
                else if (BRANCH_NAME == "develop")
                {
                    dockerFactory.PushWithTag(imageName, "${imageName}:dev-${repoVersion}")
                    manager.addShortText("app:${repoVersion}")
                    dockerFactory.PushWithTag(imageName,"${imageName}:dev-latest")

                    dockerFactory.PushWithTag(imageName2, "${imageName2}:dev-${repoVersion}")
                    manager.addShortText("bbdd:${repoVersion}")
                    dockerFactory.PushWithTag(imageName2,"${imageName2}:dev-latest")

                    dockerFactory.PushWithTag(imageName3, "${imageName3}:dev-${repoVersion}")
                    manager.addShortText("nginx:${repoVersion}")
                    dockerFactory.PushWithTag(imageName3,"${imageName3}:dev-latest")
                }
            }
        }
        else
        {
            log.Info("Branch \'${BRANCH_NAME}\' not eligible for CI/CD purposes")
        }
    }
    catch(all)
    {
        job.ErrorWithEmail("rmansillasoto@gmail.com", "Build failed, Unknown Error")
        
    }
    finally
    {
        try
        {
            dockerFactory.RemoveImage("${imageName}")
        }
        catch(all)
        {
            log.Warning("Omitting docker rmi")
        }

        workspace.Clean()
    }
    
}
