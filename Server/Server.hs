{-# LANGUAGE OverloadedStrings #-}

import System.Environment (getArgs)

import qualified Data.Map as Map (Map(..), toList, empty, insert)

import Network (withSocketsDo)
import Network.Socket
import Network.URI (uriPath)
import Network.HTTP (receiveHTTP, respondHTTP, defaultUserAgent, mkHeader, hstreamToConnection, Request(..), Response(..), HeaderName(..), Header(..), RequestMethod(..))
import qualified Network.TCP as TCP (openSocketStream, close, HandleStream(..))

import Control.Concurrent (forkIO, putMVar, takeMVar, readMVar, newEmptyMVar, MVar(..))
import Control.Monad (forever)

type Games = MVar (Map.Map String String)

main :: IO ()
main = withSocketsDo $ do 
    args <- getArgs
    let pORT = getPort args
    let anyAddr = SockAddrInet pORT iNADDR_ANY
    sock <- socket AF_INET Stream defaultProtocol
    setSocketOption sock ReuseAddr 1
    bind sock anyAddr
    listen sock sOMAXCONN

    addr <- getSocketName sock
    (host, service) <- getNameInfo [] True True addr
    print $ (\x y -> x ++ ":" ++ y) <$> host <*> service
    
    games <- newEmptyMVar
    putMVar games Map.empty    

    forever $ sockLoop games sock
    close sock

sockLoop :: Games -> Socket -> IO ()
sockLoop games sock = do 
    (newSock, remoteAddr) <- accept sock
    (host, service) <- getNameInfo [] True True remoteAddr
    processHTTP games newSock $ getHostPort host service
    
processHTTP :: Games -> Socket -> Maybe (String, Int) -> IO ()
processHTTP _ sock Nothing = (print "Invalid address!") >> (close sock)
processHTTP games sock (Just (host, port)) = do
    forkIO $ do
        stream <- TCP.openSocketStream host port sock :: IO (TCP.HandleStream String)
        request <- receiveHTTP stream 
        case request of 
            (Left err) -> print "ERROR!" >> print err
            (Right rq) -> processHTTPRequest games (host ++ ":" ++ (show port)) rq stream
        TCP.close stream
    return ()

processHTTPRequest :: Games -> String -> Request String -> TCP.HandleStream String -> IO ()
processHTTPRequest games address rq stream = do
    putStrLn $ (show rq) ++ (rqBody rq) ++ "\n\r"
    case rqMethod rq of
        GET -> respondGET games stream path
        POST -> respondPOST games address stream path $ rqBody rq
        _ -> badRespond stream
    where 
        path = (uriPath . rqURI) rq        

respondGET :: Games -> TCP.HandleStream String -> String -> IO ()
respondGET games stream path
    | path == "/games" = (readMVar games) >>= (\gamesVar -> respondHTTP stream $ successResponse gamesVar)
    | otherwise = badRespond stream
    where 
        successResponse var = Response { rspCode = (2, 0, 0)
                                       , rspReason = "OK"
                                       , rspHeaders = headers $ length $ body var
                                       , rspBody = body var
                                       }
        body var = foldl concatBody "" $ Map.toList $ var
        concatBody result (addr, params) = concat [result, addr, " ", params, "\n"]

respondPOST :: Games -> String -> TCP.HandleStream String -> String -> String -> IO ()
respondPOST games address stream path body
    | path == "/create" = (takeMVar games) 
                          >>= (\gamesVar -> putMVar games $ Map.insert address body gamesVar) 
                          >> (respondHTTP stream successResponse)
    | otherwise = badRespond stream
    where
        successResponse = Response { rspCode = (2, 0, 1)
                                   , rspReason = "Created"
                                   , rspHeaders = headers 0
                                   , rspBody = []
                                   }

badRespond :: TCP.HandleStream String -> IO ()
badRespond stream = (print "Bad Request") >> (respondHTTP stream badResponse)  

badResponse :: Response String
badResponse = let body = "Sorry, you are not welcome :(" in
     Response { rspCode = (4, 0, 0)
              , rspReason = "Bad Request"
              , rspHeaders = headers $ length body
              , rspBody = body
              }

headers :: Int -> [Header]
headers length = [ mkHeader HdrUserAgent defaultUserAgent
                 , mkHeader HdrContentType "text/plain"
                 , mkHeader HdrContentLength $ show length                  
                 ]

getPort :: [String] -> PortNumber
getPort [] = aNY_PORT
getPort (x:_) = fromInteger $ (read x :: Integer)

getHostPort :: Maybe String -> Maybe String -> Maybe (String, Int)
getHostPort (Just host) (Just service) = Just (host, read service)
getHostPort _ _ = Nothing
